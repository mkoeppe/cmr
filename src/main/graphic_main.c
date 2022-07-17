#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <cmr/matrix.h>
#include <cmr/graphic.h>
#include <cmr/graph.h>
#include <cmr/network.h>

typedef enum
{
  FILEFORMAT_UNDEFINED = 0,       /**< Whether the file format of input/output was defined by the user. */
  FILEFORMAT_MATRIX_DENSE = 1,    /**< Dense matrix format. */
  FILEFORMAT_MATRIX_SPARSE = 2,   /**< Sparse matrix format. */
  FILEFORMAT_GRAPH_EDGELIST = 3,  /**< Edge list digraph format. */
  FILEFORMAT_GRAPH_DOT = 4,       /**< Dot digraph format. */
} FileFormat;

/**
 * \brief Prints the usage of the \p program to stdout.
 * 
 * \returns \c EXIT_FAILURE.
 */

int printUsage(const char* program)
{
  printf("Usage: %s [OPTION]... FILE\n\n", program);
  puts("Converts graph to (co)graphic matrix or tests if matrix is (co)graphic, depending on input FILE.");
  puts("Options:");
  puts("  -i FORMAT  Format of input FILE; default: `dense'.");
  puts("  -o FORMAT  Format of output; default: `edgelist' if input is a matrix and `dense' if input is a graph.");
  puts("  -t         Tests for being / converts to cographic matrix.");
  puts("  -n         Output the elements of a minimal non-(co)graphic submatrix.");
  puts("  -N         Output a minimal non-(co)graphic submatrix.");
  puts("  -s         Print statistics about the computation to stderr.");
  puts("Formats for matrices: dense, sparse");
  puts("Formats for graphs: edgelist, dot (output only)");
  puts("If FILE is `-', then the input will be read from stdin.");
  return EXIT_FAILURE;
}

/**
 * \brief Converts matrix from a file to a graph if the former is (co)graphic.
 */

CMR_ERROR matrixToGraph(
  const char* instanceFileName,   /**< File name containing the input matrix (may be `-' for stdin). */
  FileFormat inputFormat,         /**< Format of the input matrix. */
  FileFormat outputFormat,        /**< Format of the output graph. */
  bool cographic,                 /**< Whether the input shall be checked for being cographic instead of graphic. */
  bool outputNonGraphicElements,  /**< Whether to print the elements of a minimal non-(co)graphic submatrix. */
  bool outputNonGraphicMatrix,    /**< Whether to print a minimal non-(co)graphic submatrix. */
  bool printStats                 /**< Whether to print statistics to stderr. */
)
{
  clock_t readClock = clock();
  FILE* instanceFile = strcmp(instanceFileName, "-") ? fopen(instanceFileName, "r") : stdin;
  if (!instanceFile)
    return CMR_ERROR_INPUT;

  CMR* cmr = NULL;
  CMR_CALL( CMRcreateEnvironment(&cmr) );

  /* Read matrix. */

  CMR_CHRMAT* matrix = NULL;
  if (inputFormat == FILEFORMAT_MATRIX_DENSE)
    CMR_CALL( CMRchrmatCreateFromDenseStream(cmr, instanceFile, &matrix) );
  else if (inputFormat == FILEFORMAT_MATRIX_SPARSE)
    CMR_CALL( CMRchrmatCreateFromSparseStream(cmr, instanceFile, &matrix) );
  if (instanceFile != stdin)
    fclose(instanceFile);
  fprintf(stderr, "Read %lux%lu matrix with %lu nonzeros in %f seconds.\n", matrix->numRows, matrix->numColumns,
    matrix->numNonzeros, (clock() - readClock) * 1.0 / CLOCKS_PER_SEC);

  /* Test for (co)graphicness. */

  bool isCoGraphic;
  CMR_GRAPH* graph = NULL;
  CMR_GRAPH_EDGE* rowEdges = NULL;
  CMR_GRAPH_EDGE* columnEdges = NULL;
  bool* edgesReversed = NULL;

  CMR_SUBMAT* submatrix = NULL;

  CMR_GRAPHIC_STATISTICS stats;
  CMR_CALL( CMRstatsGraphicInit(&stats) );
  if (cographic)
  {
    CMR_CALL( CMRtestCographicMatrix(cmr, matrix, &isCoGraphic, &graph, &rowEdges, &columnEdges,
      (outputNonGraphicElements || outputNonGraphicMatrix) ? &submatrix : NULL, &stats) );
  }
  else
  {
    CMR_CALL( CMRtestGraphicMatrix(cmr, matrix, &isCoGraphic, &graph, &rowEdges, &columnEdges,
      (outputNonGraphicElements || outputNonGraphicMatrix) ? &submatrix : NULL, &stats) );
  }

  fprintf(stderr, "Matrix %s%sgraphic.\n", isCoGraphic ? "IS " : "IS NOT ", cographic ? "co" : "");
  if (printStats)
    CMR_CALL( CMRstatsGraphicPrint(stderr, &stats, NULL) );

  if (isCoGraphic)
  {
    if (outputFormat == FILEFORMAT_GRAPH_EDGELIST)
    {
      if (cographic)
      {
        for (size_t column = 0; column < matrix->numColumns; ++column)
        {
          CMR_GRAPH_EDGE e = rowEdges[column];
          CMR_GRAPH_NODE u = CMRgraphEdgeU(graph, e);
          CMR_GRAPH_NODE v = CMRgraphEdgeV(graph, e);
          if (edgesReversed && edgesReversed[e])
          {
            CMR_GRAPH_NODE temp = u;
            u = v;
            v = temp;
          }
          printf("%d %d c%ld\n", u, v, column+1);
        }
        for (size_t row = 0; row < matrix->numRows; ++row)
        {
          CMR_GRAPH_EDGE e = columnEdges[row];
          CMR_GRAPH_NODE u = CMRgraphEdgeU(graph, e);
          CMR_GRAPH_NODE v = CMRgraphEdgeV(graph, e);
          if (edgesReversed && edgesReversed[e])
          {
            CMR_GRAPH_NODE temp = u;
            u = v;
            v = temp;
          }
          printf("%d %d r%ld\n", u, v, row+1);
        }
      }
      else
      {
        for (size_t row = 0; row < matrix->numRows; ++row)
        {
          CMR_GRAPH_EDGE e = rowEdges[row];
          CMR_GRAPH_NODE u = CMRgraphEdgeU(graph, e);
          CMR_GRAPH_NODE v = CMRgraphEdgeV(graph, e);
          if (edgesReversed && edgesReversed[e])
          {
            CMR_GRAPH_NODE temp = u;
            u = v;
            v = temp;
          }
          printf("%d %d r%ld\n", u, v, row+1);
        }
        for (size_t column = 0; column < matrix->numColumns; ++column)
        {
          CMR_GRAPH_EDGE e = columnEdges[column];
          CMR_GRAPH_NODE u = CMRgraphEdgeU(graph, e);
          CMR_GRAPH_NODE v = CMRgraphEdgeV(graph, e);
          if (edgesReversed && edgesReversed[e])
          {
            CMR_GRAPH_NODE temp = u;
            u = v;
            v = temp;
          }
          printf("%d %d c%ld\n", u, v, column+1);
        }
      }
    }
    else if (outputFormat == FILEFORMAT_GRAPH_DOT)
    {
      char buffer[16];
      puts("graph G {");
      for (size_t row = 0; row < matrix->numRows; ++row)
      {
        CMR_GRAPH_EDGE e = rowEdges[row];
        CMR_GRAPH_NODE u = CMRgraphEdgeU(graph, e);
        CMR_GRAPH_NODE v = CMRgraphEdgeV(graph, e);
        if (edgesReversed && edgesReversed[e])
        {
          CMR_GRAPH_NODE temp = u;
          u = v;
          v = temp;
        }
        printf(" v_%d -- v_%d [label=\"%s\",style=bold,color=red];\n", u, v,
          CMRelementString(CMRrowToElement(row), buffer));
      }
      for (size_t column = 0; column < matrix->numColumns; ++column)
      {
        CMR_GRAPH_EDGE e = columnEdges[column];
        CMR_GRAPH_NODE u = CMRgraphEdgeU(graph, e);
        CMR_GRAPH_NODE v = CMRgraphEdgeV(graph, e);
        if (edgesReversed && edgesReversed[e])
        {
          CMR_GRAPH_NODE temp = u;
          u = v;
          v = temp;
        }
        printf(" v_%d -- v_%d [label=\"%s\"];\n", u, v, CMRelementString(CMRcolumnToElement(column), buffer));
      }
      puts("}");
    }

    if (edgesReversed)
      CMR_CALL( CMRfreeBlockArray(cmr, &edgesReversed) );
    CMR_CALL( CMRfreeBlockArray(cmr, &rowEdges) );
    CMR_CALL( CMRfreeBlockArray(cmr, &columnEdges) );
    CMR_CALL( CMRgraphFree(cmr, &graph) );
  }

  if (submatrix && outputNonGraphicElements)
  {
    assert(submatrix);

    fprintf(stderr, "\nMinimal non-%sgraphic submatrix consists of these elements of the input matrix:\n",
      cographic ? "co" : "");
    printf("%ld rows:", submatrix->numRows);
    for (size_t r = 0; r < submatrix->numRows; ++r)
      printf(" %ld", submatrix->rows[r]+1);
    printf("\n%ld columns: ", submatrix->numColumns);
    for (size_t c = 0; c < submatrix->numColumns; ++c)
      printf(" %ld", submatrix->columns[c]+1);
    printf("\n");

    CMR_CALL( CMRsubmatFree(cmr, &submatrix) );
  }

  if (submatrix && outputNonGraphicMatrix)
  {
    CMR_CHRMAT* violatorMatrix = NULL;
    CMR_CALL( CMRchrmatZoomSubmat(cmr, matrix, submatrix, &violatorMatrix) );
    fprintf(stderr, "\nMinimal %lux%lu non-%sgraphic matrix with %lu nonzeros.\n", violatorMatrix->numRows,
      violatorMatrix->numColumns, cographic ? "co" : "", violatorMatrix->numNonzeros);
    if (inputFormat == FILEFORMAT_MATRIX_DENSE)
      CMR_CALL( CMRchrmatPrintDense(cmr, violatorMatrix, stdout, '0', false) );
    else if (inputFormat == FILEFORMAT_MATRIX_SPARSE)
      CMR_CALL( CMRchrmatPrintSparse(cmr, violatorMatrix, stdout) );
    CMR_CALL( CMRchrmatFree(cmr, &violatorMatrix) );
  }

  CMR_CALL( CMRsubmatFree(cmr, &submatrix) );
  CMR_CALL( CMRchrmatFree(cmr, &matrix) );

  /* Cleanup. */

  CMR_CALL( CMRfreeEnvironment(&cmr) );

  return CMR_OKAY;
}

/**
 * \brief Converts the given graph file to the corresponding (co)graphic matrix.
 */

CMR_ERROR graphToMatrix(
  const char* instanceFileName, /**< File name containing the input graph (may be `-' for stdin). */
  FileFormat inputFormat,       /**< Format of the input graph. */
  FileFormat outputFormat,      /**< Format of the output matrix. */
  bool cographic                /**< Whether the output shall be the cographic matrix instead of the graphic matrix. */
)
{
  FILE* instanceFile = strcmp(instanceFileName, "-") ? fopen(instanceFileName, "r") : stdin;
  if (!instanceFile)
    return CMR_ERROR_INPUT;

  CMR* cmr = NULL;
  CMR_CALL( CMRcreateEnvironment(&cmr) );

  /* Read edge list. */

  CMR_GRAPH* graph = NULL;
  CMR_ELEMENT* edgeElements = NULL;
  if (inputFormat == FILEFORMAT_GRAPH_EDGELIST)
  {
    CMR_CALL( CMRgraphCreateFromEdgeList(cmr, &graph, &edgeElements, NULL, instanceFile) );
  }
  if (instanceFile != stdin)
    fclose(instanceFile);

  /* Scan edges for (co)forest edges. */
  size_t numForestEdges = 0;
  size_t numCoforestEdges = 0;
  for (CMR_GRAPH_ITER i = CMRgraphEdgesFirst(graph); CMRgraphEdgesValid(graph, i); i = CMRgraphEdgesNext(graph, i))
  {
    CMR_GRAPH_EDGE e = CMRgraphEdgesEdge(graph, i);
    CMR_ELEMENT element = edgeElements[e];
    if (CMRelementIsRow(element))
      numForestEdges++;
    else if (CMRelementIsColumn(element))
      numCoforestEdges++;
  }

  /* Create list of (co)forest edges. */
  CMR_GRAPH_EDGE* forestEdges = NULL;
  CMR_CALL( CMRallocBlockArray(cmr, &forestEdges, numForestEdges) );
  for (size_t i = 0; i < numForestEdges; ++i)
    forestEdges[i] = -1;
  CMR_GRAPH_EDGE* coforestEdges = NULL;
  CMR_CALL( CMRallocBlockArray(cmr, &coforestEdges, numCoforestEdges) );
  for (size_t i = 0; i < numCoforestEdges; ++i)
    coforestEdges[i] = -1;

  for (CMR_GRAPH_ITER i = CMRgraphEdgesFirst(graph); CMRgraphEdgesValid(graph, i); i = CMRgraphEdgesNext(graph, i))
  {
    CMR_GRAPH_EDGE e = CMRgraphEdgesEdge(graph, i);
    CMR_ELEMENT element = edgeElements[e];
    if (CMRelementIsRow(element))
    {
      size_t rowIndex = CMRelementToRowIndex(element);
      if (rowIndex < numForestEdges)
        forestEdges[rowIndex] = e;
    }
    else if (CMRelementIsColumn(element))
    {
      size_t columnIndex = CMRelementToColumnIndex(element);
      if (columnIndex < numCoforestEdges)
        coforestEdges[columnIndex] = e;
    }
  }

  CMR_CHRMAT* matrix = NULL;
  bool isCorrectForest = false;

  clock_t startTime = clock();

  if (cographic)
  {
    CMR_CALL( CMRcomputeGraphicMatrix(cmr, graph, NULL, &matrix, numForestEdges, forestEdges, numCoforestEdges,
      coforestEdges, &isCorrectForest) );
  }
  else
  {
    CMR_CALL( CMRcomputeGraphicMatrix(cmr, graph, &matrix, NULL, numForestEdges, forestEdges, numCoforestEdges,
      coforestEdges, &isCorrectForest) );
  }

  clock_t endTime = clock();
  fprintf(stderr, "Time: %f\n", (endTime - startTime) * 1.0 / CLOCKS_PER_SEC);

  if (outputFormat == FILEFORMAT_MATRIX_DENSE)
    CMR_CALL( CMRchrmatPrintDense(cmr, matrix, stdout, '0', false) );
  else if (outputFormat == FILEFORMAT_MATRIX_SPARSE)
    CMR_CALL( CMRchrmatPrintSparse(cmr, matrix, stdout) );
  else
    assert(false);

  CMR_CALL( CMRchrmatFree(cmr, &matrix) );

  free(coforestEdges);
  free(forestEdges);
  free(edgeElements);
  CMR_CALL( CMRgraphFree(cmr, &graph) );

  CMR_CALL( CMRfreeEnvironment(&cmr) );

  return CMR_OKAY;
}

int main(int argc, char** argv)
{
  FileFormat inputFormat = FILEFORMAT_UNDEFINED;
  FileFormat outputFormat = FILEFORMAT_UNDEFINED;
  bool transposed = false;
  char* instanceFileName = NULL;
  bool outputNonGraphicElements = false;
  bool outputNonGraphicMatrix = false;
  bool printStats = false;
  for (int a = 1; a < argc; ++a)
  {
    if (!strcmp(argv[a], "-h"))
    {
      printUsage(argv[0]);
      return EXIT_SUCCESS;
    }
    else if (!strcmp(argv[a], "-t"))
      transposed = true;
    else if (!strcmp(argv[a], "-n"))
      outputNonGraphicElements = true;
    else if (!strcmp(argv[a], "-N"))
      outputNonGraphicMatrix = true;
    else if (!strcmp(argv[a], "-s"))
      printStats = true;
    else if (!strcmp(argv[a], "-i") && a+1 < argc)
    {
      if (!strcmp(argv[a+1], "dense"))
        inputFormat = FILEFORMAT_MATRIX_DENSE;
      else if (!strcmp(argv[a+1], "sparse"))
        inputFormat = FILEFORMAT_MATRIX_SPARSE;
      else if (!strcmp(argv[a+1], "edgelist"))
        inputFormat = FILEFORMAT_GRAPH_EDGELIST;
      else
      {
        printf("Error: unknown input file format <%s>.\n\n", argv[a+1]);
        return printUsage(argv[0]);
      }
      ++a;
    }
    else if (!strcmp(argv[a], "-o") && a+1 < argc)
    {
      if (!strcmp(argv[a+1], "dense"))
        outputFormat = FILEFORMAT_MATRIX_DENSE;
      else if (!strcmp(argv[a+1], "sparse"))
        outputFormat = FILEFORMAT_MATRIX_SPARSE;
      else if (!strcmp(argv[a+1], "edgelist"))
        outputFormat = FILEFORMAT_GRAPH_EDGELIST;
      else if (!strcmp(argv[a+1], "dot"))
        outputFormat = FILEFORMAT_GRAPH_DOT;
      else
      {
        printf("Error: unknown output format <%s>.\n\n", argv[a+1]);
        return printUsage(argv[0]);
      }
      ++a;
    }
    else if (!instanceFileName)
      instanceFileName = argv[a];
    else
    {
      printf("Error: Two input files <%s> and <%s> specified.\n\n", instanceFileName, argv[a]);
      return printUsage(argv[0]);
    }
  }

  if (!instanceFileName)
  {
    puts("No input file specified.\n");
    return printUsage(argv[0]);
  }

  if (inputFormat == FILEFORMAT_UNDEFINED)
  {
    if (outputFormat == FILEFORMAT_UNDEFINED)
    {
      inputFormat = FILEFORMAT_MATRIX_DENSE;
      outputFormat = FILEFORMAT_GRAPH_EDGELIST;
    }
    else if (outputFormat == FILEFORMAT_MATRIX_DENSE || outputFormat == FILEFORMAT_MATRIX_SPARSE)
      inputFormat = FILEFORMAT_GRAPH_EDGELIST;
    else
      inputFormat = FILEFORMAT_MATRIX_DENSE;
  }
  else if (inputFormat == FILEFORMAT_MATRIX_DENSE || inputFormat == FILEFORMAT_MATRIX_SPARSE)
  {
    if (outputFormat == FILEFORMAT_UNDEFINED)
      outputFormat = FILEFORMAT_GRAPH_EDGELIST;
    else if (outputFormat == FILEFORMAT_MATRIX_DENSE || outputFormat == FILEFORMAT_MATRIX_SPARSE)
    {
      puts("Either input or output must be a graph.\n");
      return printUsage(argv[0]);
    }
  }
  else
  {
    /* Input format is graph format. */
    if (outputFormat == FILEFORMAT_UNDEFINED)
      outputFormat = FILEFORMAT_MATRIX_DENSE;
    else if (!(outputFormat == FILEFORMAT_MATRIX_DENSE || outputFormat == FILEFORMAT_MATRIX_SPARSE))
    {
      puts("Either input or output must be a matrix.\n");
      return printUsage(argv[0]);
    }
  }

  CMR_ERROR error;
  if (inputFormat == FILEFORMAT_MATRIX_DENSE || inputFormat == FILEFORMAT_MATRIX_SPARSE)
  {
    error = matrixToGraph(instanceFileName, inputFormat, outputFormat, transposed, outputNonGraphicElements,
      outputNonGraphicMatrix, printStats);
  }
  else
    error = graphToMatrix(instanceFileName, inputFormat, outputFormat, transposed);

  switch (error)
  {
  case CMR_ERROR_INPUT:
    puts("Input error.");
    return EXIT_FAILURE;
  case CMR_ERROR_MEMORY:
    puts("Memory error.");
    return EXIT_FAILURE;
  default:
    return EXIT_SUCCESS;
  }
}
