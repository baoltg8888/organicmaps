#pragma once

#include "routing_common/num_mwm_id.hpp"

#include "geometry/mercator.hpp"
#include "geometry/tree4d.hpp"

#include <cmath>
#include <limits>
#include <unordered_set>

namespace routing
{
// Each MWM may have two canonical bounds. Query both longitude halves and emit
// each ID once, including countries whose rectangles only touch the query.
template <typename Fn>
void ForEachNumMwmInRect(m4::Tree<NumMwmId> const & tree, m2::RectD const & rect, Fn && fn)
{
  std::unordered_set<NumMwmId> visited;
  mercator::ForEachRectXWrapped(rect, [&](m2::RectD const & part)
  {
    // m4::Tree uses open intersection tests; routing bounds are closed.
    double constexpr kInfinity = std::numeric_limits<double>::infinity();
    m2::RectD const query(std::nextafter(part.minX(), -kInfinity), std::nextafter(part.minY(), -kInfinity),
                          std::nextafter(part.maxX(), kInfinity), std::nextafter(part.maxY(), kInfinity));
    tree.ForEachInRect(query, [&](NumMwmId id)
    {
      if (visited.insert(id).second)
        fn(id);
    });
  });
}
}  // namespace routing
