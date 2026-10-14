#include "testing/testing.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "routing_common/num_mwm_tree.hpp"

#include <algorithm>
#include <vector>

namespace storage_routing_helpers_tests
{
class Getter : public storage::CountryInfoGetterForTesting
{
public:
  explicit Getter(std::vector<storage::CountryDef> const & countries) : CountryInfoGetterForTesting(countries)
  {
    for (size_t i = 0; i < m_countries.size(); ++i)
      m_countryIndex.emplace(m_countries[i].m_countryId, i);
  }
};

UNIT_TEST(NumMwmTree_WrappedBounds)
{
  std::vector<storage::CountryDef> const countries = {{"East", {170, -10, 190, 10}},
                                                      {"West", {-190, 20, -170, 30}},
                                                      {"Offset", {184, 40, 193, 50}},
                                                      {"Canonical", {-10, -10, 10, 10}},
                                                      {"Touch", {10, -10, 20, 10}},
                                                      {"AllLongitudes", {-180.000001, -50, 180.000001, 60}},
                                                      {"ShiftedWorld", {-540, -50, -180, 60}},
                                                      {"Zero", {0, 0, 1, 1}}};
  Getter getter(countries);
  routing::NumMwmIds ids;
  for (auto const & country : countries)
    ids.RegisterFile(platform::CountryFile(country.m_countryId));
  auto const tree = routing::MakeNumMwmTree(ids, getter);
  for (auto const & query :
       {m2::RectD(-175, 0, -175, 0), m2::RectD(185, 0, 185, 0), m2::RectD(545, 0, 545, 0), m2::RectD(-535, 0, -535, 0),
        m2::RectD(175, 25, 175, 25), m2::RectD(-175, 25, -175, 25), m2::RectD(-170, 45, -170, 45),
        m2::RectD(170, -10, 190, 30), m2::RectD(-190, -10, -170, 30), m2::RectD(180, -10, 180, 30),
        m2::RectD(-180, -10, -180, 30), m2::RectD(10, 0, 10, 0), m2::RectD(-180, -50, 180, 60),
        m2::RectD(-720, -60, 720, 70), m2::RectD(0, 61, 0, 61), m2::RectD(0, 0, 0, 0)})
  {
    std::vector<routing::NumMwmId> expected;
    for (routing::NumMwmId id = 0; id < countries.size(); ++id)
    {
      storage::CountryDef const country(countries[id].m_countryId, mercator::WrapRectX(countries[id].m_rect));
      if (country.IsRectOverlap(mercator::WrapRectX(query)))
        expected.push_back(id);
    }
    std::vector<routing::NumMwmId> actual;
    routing::ForEachNumMwmInRect(*tree, query, [&](routing::NumMwmId id) { actual.push_back(id); });
    std::sort(actual.begin(), actual.end());
    TEST_EQUAL(actual, expected, (query));
  }
}
}  // namespace storage_routing_helpers_tests
