#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <vds/vds.h>
#include <iostream>
#include <cstdlib>
#include <string>

// we need to test against cloud if we want to assure our dependencies
// do not leak as local files behave differently from cloud
const char* storage_account_url = std::getenv("STORAGE_ACCOUNT_URL");
const char* sas = std::getenv("SAS");const std::string url =
  "azure://testdata/wellknown/well_known_default";
const std::string nonexistent_vds_url = "azure://testdata/wellknown/unexisting";
const std::string prestack_url = "azure://testdata/prestack/prestack_default";
const std::string broken_vds_structure_url =
  "azure://testdata/wellknown/well_known_broken_structure";

// connection must be in form of BlobEndpoint/SAS
// AccountName/AccountKey seem to leak in valid cases
const std::string connection =
  "BlobEndpoint="+std::string(storage_account_url)+";SharedAccessSignature="+std::string(sas);
const std::string invalid_connection=
  "BlobEndpoint="+std::string(storage_account_url)+";SharedAccessSignature=wrong";

TEST_CASE("No our call causes memory leak") {

    vdsbuffer result;
    SECTION( "slice call " ) {
        SECTION( "valid slice" ) {
            result = slice(url.c_str(), connection.c_str(), 3, INLINE);
            CHECK(result.size != 0);
        }
        SECTION( "invalid slice lineno (any our error after openvds.Open)" ) {
            result = slice(url.c_str(), connection.c_str(), 30, INLINE);
            CHECK(result.size == 0);
        }
    }

    SECTION( "slice metadata call " ) {
        SECTION( "valid slice metadata" ) {
            result = slice_metadata(url.c_str(), connection.c_str(), 3, INLINE);
            CHECK(result.size != 0);
        }
        SECTION( "prestack data (any our error)" ) {
            result = slice_metadata(prestack_url.c_str(), connection.c_str(), 2, INLINE);
            CHECK(result.size == 0);
        }
    }

    SECTION( "fence call " ) {
        const float coords[2] = {3.0,6.2};

        SECTION( "valid fence" ) {
            result = fence(url.c_str(), connection.c_str(), "cdp", coords, 1, LINEAR);
            CHECK(result.size != 0);
        }
        SECTION( "uknown coordinate system (any our error)" ) {
            result = fence(url.c_str(), connection.c_str(), "chicken", coords, 1, LINEAR);
            CHECK(result.size == 0);
        }
    }

    SECTION( "metadata call " ) {
        SECTION( "valid metadata" ) {
            result = metadata(url.c_str(), connection.c_str());
            CHECK(result.size != 0);
        }
    }

    vdsbuffer_delete(&result);
}

TEST_CASE("openvds targeted, leaking somewhere", "[openvdsleak]" ) {
    vdsbuffer result;
    const float coords[2] = {3.0,6.2};

    SECTION( "invalid authentication" ) {
        result = fence(url.c_str(), invalid_connection.c_str(), "cdp", coords, 1, LINEAR);
        CHECK(result.size == 0);
    }
    SECTION( "invalid vds path" ) {
        result = metadata(nonexistent_vds_url.c_str(), connection.c_str());
        CHECK(result.size == 0);
    }

    SECTION( "vds structure is bad", "[openvdsleak]" ) {
        // we can't test all openvds issues, so test random openvds exception
        result = slice(broken_vds_structure_url.c_str(), connection.c_str(), 3, INLINE);
        std::cout << result.err << std::flush;
        CHECK(result.size == 0);
    }
    vdsbuffer_delete(&result);
}
