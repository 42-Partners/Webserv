#include "tests.hpp"

#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
	if (argc > 1 && std::strcmp(argv[1], "-v") == 0)
		g_verbose = true;

	RUN(testInitialState);

	// request line
	RUN(testValidRequest);
	RUN(testSupportedMethods);
	RUN(testRequestLineErrors);
	RUN(testUriErrors);
	RUN(testUriAccepted);
	RUN(testQueryString);
	RUN(testHttpVersions);
	RUN(testIncompleteLine);
	RUN(testIncompleteLineCrOnly);
	RUN(testBareLfDoesNotTerminate);
	RUN(testRequestLineInPieces);
	RUN(testAdditionalData);
	RUN(testEmptyLinesBeforeRequest);
	RUN(testRequestLineLengthBoundary);

	// headers
	RUN(testValidHeaders);
	RUN(testHeaderErrors);
	RUN(testHeadersAccepted);
	RUN(testHostAccepted);
	RUN(testHeaderNameIsCaseInsensitive);
	RUN(testConnectionHeader);
	RUN(testHeadersInPieces);
	RUN(testHeadersByteByByte);
	RUN(testHeadersWithoutTerminatorWaits);
	RUN(testHeaderCountBoundary);
	RUN(testHeadersSizeBoundary);

	// Content-Length / Transfer-Encoding
	RUN(testContentLengthErrors);
	RUN(testContentLengthOverflow);
	RUN(testContentLengthSizeMax);
	RUN(testContentLengthHeaderOnly);
	RUN(testTransferEncodingValues);
	RUN(testTransferEncodingOnGet);

	// body com Content-Length
	RUN(testNoBodyKeepsExtraBytes);
	RUN(testBodyContentLengthComplete);
	RUN(testBodyContentLengthLeadingZeros);
	RUN(testBodyContentLengthHeadersThenBody);
	RUN(testBodyContentLengthInPieces);
	RUN(testBodyContentLengthByteByByte);
	RUN(testBodyContentLengthExtraBytes);
	RUN(testBodyContentLengthBinary);
	RUN(testBodyContentLengthAllByteValues);
	RUN(testBodyContentLengthMissingBytes);
	RUN(testZeroContentLengthKeepsExtraBytes);
	RUN(testBodyContentLengthLarge);

	// limite de body
	RUN(testDefaultLimitBoundary);
	RUN(testCustomLimits);

	// chunked
	RUN(testChunkedAccepted);
	RUN(testChunkedHeadersThenBody);
	RUN(testChunkedPartialStates);
	RUN(testChunkedByteByByte);
	RUN(testChunkedExtraBytes);
	RUN(testChunkedErrors);
	RUN(testChunkedLimitErrors);
	RUN(testChunkedSizeLineLimits);
	RUN(testChunkedSizeOverflow);
	RUN(testChunkedTrailerSizeLimits);
	RUN(testChunkedManySmallChunks);
	RUN(testChunkedLargeInPieces);

	// transversais
	RUN(testNulBytes);
	RUN(testCompleteIsSticky);
	RUN(testErrorIsSticky);
	RUN(testPipelinedRequests);
	RUN(testFragmentation);

	// StatusCode
    RUN(testEnumValues);
    RUN(testReasonPhrases);
    RUN(testNoCodeFallsBackToUnknown);
    RUN(testUnknownCodes);
    RUN(testDefaultErrorPage);
    RUN(testDefaultErrorPageStructure);
    RUN(testDefaultErrorPageUnknownCode);

	std::cout << "\n" << g_passed << " passaram, " << g_failed << " falharam" << std::endl;

	return g_failed == 0 ? 0 : 1;
}