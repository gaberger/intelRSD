/*!
 * @brief Watcher
 *
 * Class handling events and executing periodic tasks.
 * @todo To be splited into several classes: EventProcessor, TaskSheduler, tasks
 *
 * @header{License}
 * @copyright Copyright (c) 2016-2017 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @header{Filesystem}
 * @file watcher.cpp
 */

#include "psme/rest/model/watcher.hpp"

#include "psme/core/agent/agent_manager.hpp"
#include "psme/rest/model/handlers/root_handler.hpp"
#include "agent-framework/eventing/events_queue.hpp"

#include "configuration/configuration.hpp"

namespace psme {
namespace rest {
namespace model {

class PollingTask : public WatcherTask {
public:
    PollingTask();

    /*!
     * @brief Get polling interval.
     * @return always same value, taken from config file
     */
    std::chrono::seconds get_interval() const override {
        return interval;
    }

    /*! @brief Poll all agents to get whole tree of data */
    void execute() override;

private:
    std::chrono::seconds interval{};

    handler::RootHandler root_handler{};
};


PollingTask::PollingTask() : WatcherTask("Polling") {
    auto config = configuration::Configuration::get_instance().to_json();
    interval = std::chrono::seconds(config["eventing"]["poll-interval-sec"].as_uint());
}

void PollingTask::execute() {
    auto agents = psme::core::agent::AgentManager::get_instance()->get_agents();
    for (auto& agent : agents) {
        try {
            auto polling = [this,agent] {
                this->root_handler.poll(agent, "" /* parent_uuid */, agent_framework::model::enums::Component::None, "" /* uuid */);
            };
            agent->execute_in_transaction(polling);
        }
        catch (const psme::core::agent::AgentUnreachable&)  {
            log_error(GET_LOGGER("rest"), "Polling failed due to agent (id:" << agent.get()->get_gami_id() << ") unreachable");
        }
    }
}

class RetentionPolicyTask : public WatcherTask {
public:
    RetentionPolicyTask();

    /*!
     * @brief Get polling interval.
     * @return always same value, taken from config file
     */
    std::chrono::seconds get_interval() const override;

    /*!
     * @brief Check persisten UUID database
     *
     * When called for first time, it invalidates all existing entries. Next
     * executions removes all outdated ones (to keep database clean).
     */
    void execute() override;

    /*!
     * @brief Invalidate all entries in the DB, and remove outdated
     */
    void started() override;

    /*!
     * @brief Invalidate all entries in the DB
     */
    void stopped() override;

private:
    std::chrono::seconds interval{0};
    std::chrono::seconds outdated{0};
};

RetentionPolicyTask::RetentionPolicyTask() : WatcherTask("RetentionPolicy") {
    auto config = configuration::Configuration::get_instance().to_json();

    auto interval_value = config["retention-policy"]["interval-sec"];
    interval = std::chrono::seconds(interval_value.is_uint() ? interval_value.as_uint() : 0);

    auto outdated_value = config["retention-policy"]["outdated-sec"];
    outdated = outdated_value.is_uint() ? std::chrono::seconds(outdated_value.as_uint()) : std::chrono::hours(24);
}

void RetentionPolicyTask::started() {
    unsigned invalidated = handler::Database::invalidate_all(outdated);
    log_info(GET_LOGGER("rest"), "Initially invalidated " << invalidated << " persistence entries");
}

std::chrono::seconds RetentionPolicyTask::get_interval() const {
    /* initial cleanup must be run immediately */
    return interval;
}

void RetentionPolicyTask::execute() {
    if (0 == interval.count()) {
        throw WatcherTask::StopWatching();
    }
    unsigned removed = handler::Database::remove_outdated(outdated);
    if (0 != removed) {
        log_info(GET_LOGGER("rest"), "Removed " << removed << " persistance entries on outdate," <<
            " interval " << outdated.count());
    }
}

void RetentionPolicyTask::stopped() {
    if (0 != interval.count()) {
        unsigned invalidated = handler::Database::invalidate_all(outdated);
        log_info(GET_LOGGER("rest"), "Invalidated " << invalidated << " persistance entries");
    }
}

Watcher::Watcher() {
    add_task(std::unique_ptr<WatcherTask>(new PollingTask()));
    add_task(std::unique_ptr<WatcherTask>(new RetentionPolicyTask()));
}

Watcher::~Watcher() { stop(); }

void Watcher::start() {
    if (!m_running) {
        m_running = true;
        m_thread = std::thread(&Watcher::watch, this);
    }
}

void Watcher::stop() {
    if (m_running) {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
            log_debug(GET_LOGGER("rest"), "Watcher job done!");
        }
    }
}

void Watcher::add_task(Watcher::WatcherTaskPtr task) {
    std::chrono::seconds interval = task->get_interval();
    if (m_running) {
        task->started();
    }
    insert({
        /* task */ std::move(task),
        /* next run */ std::chrono::steady_clock::now() + interval,
        /* executed */ 0
    });
}

void Watcher::insert(Watcher::Registration&& reg) {
    for (Registrations::iterator it = added_tasks.begin(); it != added_tasks.end(); it++) {
        /* '<=' to avoid starving 'zero interval' tasks, previous "now" are executed as first */
        if ((*it).next_run <= reg.next_run) {
            /* insert just before "earlier" task */
            added_tasks.insert(it, std::move(reg));
            return;
        }
    }
    /* nothing "earlier" found, add at the end, it will be the first to be exectued */
    added_tasks.push_back(std::move(reg));
}


void Watcher::watch() {
    Registrations::iterator it{};

    /* notify all tasks started */
    for (it = added_tasks.begin(); it != added_tasks.end(); it++) {
        it->next_run = std::chrono::steady_clock::now() + it->task->get_interval();
        it->task->started();
    }
    std::sort(added_tasks.begin(), added_tasks.end(), [](const Registration& l, const Registration& r) {
        return l.next_run > r.next_run;
    });

    while (m_running) {
        if ((!added_tasks.empty()) && (added_tasks.back().next_run <= std::chrono::steady_clock::now())) {
            /* workaround for moving unique_ptr, for move constess must be thrown away.. */
            Registration found = std::move(const_cast<Registration&>(added_tasks.back()));
            added_tasks.pop_back();

            /* should the interval be added, or execute time computed on current time? */
            found.next_run += found.task->get_interval();
            found.executed++;

            try {
                log_info(GET_LOGGER("rest"), found.task->get_name() << " started run #" << found.executed);
                auto started_at = std::chrono::high_resolution_clock::now();

                found.task->execute();

                auto finished_at = std::chrono::high_resolution_clock::now();
                auto duration = finished_at - started_at;
                log_info(GET_LOGGER("rest"), found.task->get_name() << " completed run #" << found.executed <<
                        " [" << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms]");
            }
            catch (const PollingTask::StopWatching&) {
                log_warning(GET_LOGGER("rest"), found.task->get_name() << " requested to be removed" <<
                        ", run #" << found.executed);
                /* don't push registration back! Task will be deleted when 'found' goes out of scope */
                found.task->stopped();
                continue;
            }
            catch (...) {
                log_error(GET_LOGGER("rest"), found.task->get_name() << " failed with exception" <<
                        ", run #" << found.executed);
            }

            /* add "modified" task back to the queue */
            insert(std::move(found));
        } else {
            /*! @todo events are starved and queue grows if tasks took too much time. */
            auto event = agent_framework::eventing::EventsQueue::get_instance()->
                    wait_for_and_pop(std::chrono::seconds(1));
            if (event && m_running) {
                process_event(event.get());
            }
        }
    }

    /* notify all tasks about stop */
    for (it = added_tasks.begin(); it != added_tasks.end(); it++) {
        it->task->stopped();
    }
}

void Watcher::process_event(const Watcher::EventData* const event) {
    try {
        auto agent = core::agent::AgentManager::get_instance()->get_agent(event->get_gami_id());
        if(nullptr == agent) {
            log_error(GET_LOGGER("rest"), "Agent GAMI ID " << event->get_gami_id() << " not recognized");
            return;
        }

        auto event_handling = [this,agent,event] {
            auto handler = handler::HandlerManager::get_instance()->get_handler(event->get_type());
            if (!handler->handle(agent, *event)) {
                log_info(GET_LOGGER("rest"), "Event not processed corectly. event->get_type() = "
                    << event->get_type() << "; event->get_notification() = " << event->get_notification().to_string());
            }
        };
        agent->execute_in_transaction(event_handling);

    } catch (const std::exception &error) {
        log_error(GET_LOGGER("rest"), "Event exception occured: " << error.what());
    } catch (...) {
        log_error(GET_LOGGER("rest"), "Unknown exception occured.");
    }
}

WatcherTask::WatcherTask(const std::string& _name) :
    name(_name) { }

WatcherTask::~WatcherTask() { }

WatcherTask::StopWatching::~StopWatching() { }


} /*! @i{namespace model} */
} /*! @i{namespace rest} */
} /*! @i{namespace psme} */
