#ifndef TESTS_HPP
# define TESTS_HPP

#include <string>

#define RUN(t) do { if (g_verbose) std::cout << "== " #t "\n"; t(); } while (0)
extern int  g_passed;
extern int  g_failed;
extern bool g_verbose;

void testInitialState();
void check(bool condition, const std::string& name);

// request line
void testValidRequest();
void testSupportedMethods();
void testRequestLineErrors();
void testUriErrors();
void testUriAccepted();
void testQueryString();
void testHttpVersions();
void testIncompleteLine();
void testIncompleteLineCrOnly();
void testBareLfDoesNotTerminate();
void testRequestLineInPieces();
void testAdditionalData();
void testEmptyLinesBeforeRequest();
void testRequestLineLengthBoundary();

// headers
void testValidHeaders();
void testHeaderErrors();
void testHeadersAccepted();
void testHostAccepted();
void testHeaderNameIsCaseInsensitive();
void testConnectionHeader();
void testHeadersInPieces();
void testHeadersByteByByte();
void testHeadersWithoutTerminatorWaits();
void testHeaderCountBoundary();
void testHeadersSizeBoundary();

// Content-Length / Transfer-Encoding
void testContentLengthErrors();
void testContentLengthOverflow();
void testContentLengthSizeMax();
void testContentLengthHeaderOnly();
void testTransferEncodingValues();
void testTransferEncodingOnGet();

// body com Content-Length
void testNoBodyKeepsExtraBytes();
void testBodyContentLengthComplete();
void testBodyContentLengthLeadingZeros();
void testBodyContentLengthHeadersThenBody();
void testBodyContentLengthInPieces();
void testBodyContentLengthByteByByte();
void testBodyContentLengthExtraBytes();
void testBodyContentLengthBinary();
void testBodyContentLengthAllByteValues();
void testBodyContentLengthMissingBytes();
void testZeroContentLengthKeepsExtraBytes();
void testBodyContentLengthLarge();

// limite de body
void testDefaultLimitBoundary();
void testCustomLimits();

// chunked
void testChunkedAccepted();
void testChunkedHeadersThenBody();
void testChunkedPartialStates();
void testChunkedByteByByte();
void testChunkedExtraBytes();
void testChunkedErrors();
void testChunkedLimitErrors();
void testChunkedSizeLineLimits();
void testChunkedSizeOverflow();
void testChunkedTrailerSizeLimits();
void testChunkedManySmallChunks();
void testChunkedLargeInPieces();

// transversais
void testNulBytes();
void testCompleteIsSticky();
void testErrorIsSticky();
void testPipelinedRequests();
void testFragmentation();

// erros / status codes (error.cpp)
void testEnumValues();
void testReasonPhrases();
void testNoCodeFallsBackToUnknown();
void testUnknownCodes();
void testDefaultErrorPage();
void testDefaultErrorPageStructure();
void testDefaultErrorPageUnknownCode();

#endif