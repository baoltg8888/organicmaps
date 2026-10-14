#include "testing/testing.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/feature.hpp"

#include "coding/file_reader.hpp"
#include "coding/files_container.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "defines.hpp"

namespace country_info_reader_tests
{
class Reader : public storage::CountryInfoReader
{
public:
  explicit Reader(std::string const & path) : CountryInfoReader(ModelReaderPtr(std::make_unique<FileReader>(path)))
  {
    // Three countries guarantee eviction from this two-slot cache.
    m_trgCache.Init(1);
  }
};

UNIT_TEST(CountryInfoReader_TriangleCacheEviction)
{
  auto const path = GetPlatform().TmpPathForFile();
  SCOPE_GUARD(cleanup, [&] { base::DeleteFileX(path); });
  serial::GeometryCodingParams const cp;
  std::array<std::vector<m2::PointD>, 3> expected;
  {
    FilesContainerW container(path);
    std::vector<storage::CountryDef> countries;
    for (uint32_t id = 0; id < expected.size(); ++id)
    {
      auto const name = std::to_string(id);
      double const x = id * 4.0;
      m2::PointU points[] = {PointDToPointU({x, 0}, cp.GetCoordBits()), PointDToPointU({x + 1, 0}, cp.GetCoordBits()),
                             PointDToPointU({x, 1}, cp.GetCoordBits())};
      for (auto const & point : points)
        expected[id].push_back(PointUToPointD(point, cp.GetCoordBits()));
      serial::TrianglesChainSaver saver(cp);
      saver(points, {{-1, 0, 0, static_cast<char>(-1)}});
      saver.Save(*container.GetWriter("t" + name));
      countries.emplace_back(name, m2::RectD(x, 0, x + 1, 1));
    }
    rw::Write(*container.GetWriter(PACKED_POLYGONS_INFO_TAG), countries);
  }
  Reader reader(path);
  auto check = [&]
  {
    for (uint32_t id : {0, 1, 2, 0, 1, 2})
    {
      FeatureType feature({}, 0);
      reader.GetTriangles(id, feature);
      auto const & actual = feature.GetTrianglesAsPoints(FeatureType::BEST_GEOMETRY);
      TEST_EQUAL(std::vector<m2::PointD>(actual.begin(), actual.end()), expected[id], (id));
    }
  };
  check();
  reader.ClearCaches();
  check();
}
}  // namespace country_info_reader_tests
