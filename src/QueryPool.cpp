#include "wing/QueryPool.h"
#include "wing/EventLoop.h"

namespace wing
{

QueryPool::QueryPool(ConnectionInfo connection)
    : m_lock(),
      m_queries(),
      m_connection(std::move(connection)),
      m_event_loop(nullptr)
{

}

QueryPool::QueryPool(
    ConnectionInfo connection,
    EventLoop* event_loop
)
    : m_lock(),
      m_queries(),
      m_connection(std::move(connection)),
      m_event_loop(event_loop)
{

}

auto QueryPool::GetConnection() const -> const ConnectionInfo&
{
    return m_connection;
}

auto QueryPool::Produce(
    const std::string& query,
    std::chrono::milliseconds timeout
) -> Query {
    return Produce(query, timeout, nullptr);
}

auto QueryPool::Produce(
    const std::string& query,
    std::chrono::milliseconds timeout,
    OnCompleteHandler on_complete
) -> Query
{
    m_lock.lock();
    if(m_queries.empty())
    {
        m_lock.unlock();
        QueryHandlePtr request_handle_ptr(
            new QueryHandle(
                m_event_loop,
                *this,
                m_connection,
                on_complete,
                timeout,
                query
            )
        );

        return Query(std::move(request_handle_ptr));
    }
    else
    {
        auto request_handle_ptr = std::move(m_queries.back());
        m_queries.pop_back();
        m_lock.unlock();

        request_handle_ptr->SetOnCompleteHandler(on_complete);
        request_handle_ptr->SetTimeout(timeout);
        request_handle_ptr->SetQuery(query);

        return Query(std::move(request_handle_ptr));
    }
}

auto QueryPool::returnQuery(
    QueryHandlePtr query_handle
) -> void
{
    // If the handle has had any kind of error while processing
    // simply release the memory and close it.
    // libuv will shutdown the poll/timer handles
    // and then delete the request handle.
    if(query_handle->HasError())
    {
        auto* raw = query_handle.release();
        raw->close();
        return;
    }

    {
        std::lock_guard<std::mutex> guard(m_lock);
        m_queries.emplace_back(std::move(query_handle));
    }
}

auto QueryPool::close() -> void
{
    for(auto& request_handle : m_queries)
    {
        auto* raw = request_handle.release();
        raw->close();
    }
}

} // wing
