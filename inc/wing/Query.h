#pragma once

#include "wing/QueryHandle.h"

#include <memory>

namespace wing
{

class Query
{
    friend class QueryPool;
    friend class EventLoop;
public:
    ~Query();
    Query(const Query&) = delete;                   ///< No copying
    Query(Query&& from) = default;                  ///< Can move
    auto operator = (const Query&) = delete;          ///< No copy assign
    auto operator = (Query&&) -> Query& = default;  ///< Can move assign

    auto operator * () -> QueryHandle&;
    auto operator * () const -> const QueryHandle&;
    auto operator -> () -> QueryHandle*;
    auto operator -> () const -> const QueryHandle*;
private:
    Query(
        std::unique_ptr<QueryHandle> request_handle
    );

    std::unique_ptr<QueryHandle> m_request_handle;
};

} // wing
