// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/elements/miranda/generators/spatterbench.h>

#include <sst/core/params.h>

using namespace SST::Miranda;


SpatterBenchGenerator::SpatterBenchGenerator(ComponentId_t id, Params& params) : RequestGenerator(id, params)
{
    build(params);
}

void SpatterBenchGenerator::build(Params& params)
{
    const uint32_t verbose = params.find<uint32_t>("verbose", 0);
    const std::string args = "./Spatter " + params.find<std::string>("args", "");

    char **argv = nullptr;
    int argc    = 0;
    int res     = 0;

    out = new Output("SpatterBenchGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

    warmupRuns = params.find<uint32_t>("warmup_runs", 10);
    reqLength  = sizeof(size_t);
    configIdx  = 0;
    countIdx   = 0;
    patternIdx = 0;
    warmupIdx  = 0;
    configFin  = false;
    warmupFin  = (0 == warmupRuns);
    warmupAll  = !params.find<bool>("only_warmup_first", false);

    if (0 > warmupRuns) {
        out->fatal(CALL_INFO, -1, "Value for warmup_runs must be greater than or equal to zero\n");
    }

    statReadBytes  = registerStatistic<uint64_t>( "total_bytes_read" );
    statWriteBytes = registerStatistic<uint64_t>( "total_bytes_write" );
    statReqLatency = registerStatistic<uint64_t>( "req_latency" );
    statCycles     = registerStatistic<uint64_t>( "cycles" );

    // Convert the arguments to a compatible format before parsing them.
    countArgs(args, argc);
    tokenizeArgs(args, argc, &argv);

    res = Spatter::parse_input(argc, argv, configList);


    // The allocated memory is no longer needed.
    for (int i = 0; i < argc; ++i) {
        delete [] argv[i];
    }
    delete [] argv;


    if (0 != res) {
        out->fatal(CALL_INFO, -1, "Failed to parse provided arguments\n");
    } else {
        // Output the specified arguments for each run-configuration.
        std::ostringstream oss;
        oss << configList;
        out->output("\n%s", oss.str().c_str());
    }

    out->output("\n%-15s", "config");
    out->output("%-15s",   "bytes");
    out->output("%-15s",   "time(s)");
    out->output("%-15s",   "bw(MB/s)");
    out->output("%-15s",   "cycles");
    out->output("%-15s\n", "time(s)/cycles");
}

SpatterBenchGenerator::~SpatterBenchGenerator()
{
    delete out;
}

void SpatterBenchGenerator::generate(MirandaRequestQueue<GeneratorRequest*>* q)
{
    if (!configFin) {
        const Spatter::ConfigurationBase *config = configList.configs[configIdx].get();
        queue = q;

        if (0 == config->kernel.compare("gather")) {
            gather();
        } else if (0 == config->kernel.compare("scatter")) {
            scatter();
        } else if (0 == config->kernel.compare("sg")) {
            scatterGather();
        } else if (0 == config->kernel.compare("multigather")) {
            multiGather();
        } else if (0 == config->kernel.compare("multiscatter")) {
            multiScatter();
        } else {
            out->fatal(CALL_INFO, -1, "Invalid kernel: %s\n", config->kernel.c_str());
        }

        updateIndices();
    }
}

bool SpatterBenchGenerator::isFinished()
{
    if (configFin && warmupFin) {
        const Spatter::ConfigurationBase *prevConfig = configList.configs[configIdx-1].get();
        uint64_t expectedBytes = getPatternSize(prevConfig) * prevConfig->count * reqLength;
        uint64_t recordedBytes = calcBytes(prevConfig);

        if (0 == prevConfig->kernel.compare("sg")) {
            // GS patterns expect twice the number of bytes.
            expectedBytes <<= 1;
        }

        if ((0 != warmupIdx) && (0 != warmupRuns)) {
            // Completed warm-up runs for the previous run-configuration.
            recordedBytes /= warmupRuns;
        }

        // Check if the requests associated with the previous run-configuration have been executed.
        if (recordedBytes == expectedBytes) {
            if (0 != warmupIdx) {
                // Completed warm-up runs for the previous run-configuration.
                warmupIdx = 0;
                --configIdx;
            } else {
                // The requests associated with the previous run-configuration have finished executing.
                outputStats();

                // Reset the warm-up flag after each run-configuration has been completed.
                if (warmupAll) {
                    warmupFin = (0 == warmupRuns);
                }
            }
            configFin = false;

            // Reset the statistics for the next run-configuration.
            statReadBytes->setCollectionCount(0);
            statWriteBytes->setCollectionCount(0);
            statReqLatency->setCollectionCount(0);
            statCycles->setCollectionCount(0);
        }
    }

    return (configIdx == configList.configs.size());
}

void SpatterBenchGenerator::completed()
{
    out->output("\n");
}

/**
   * @brief Counts the number of arguments in a string.
   *
   * @param args The string of arguments to be counted.
   * @param argc Number of arguments found in the string.
   */
void SpatterBenchGenerator::countArgs(const std::string &args, int32_t &argc)
{
    std::istringstream iss(args);
    std::string tok;

    while (iss >> tok) {
        ++argc;
    }
}

/**
   * @brief Tokenize a string of arguments into an array of arguments
            and allocates memory for the array of arguments.
   *
   * @param args String of arguments to be tokenized.
   * @param argc Number of arguments in the string.
   * @param argv Destination array for the arguments.
   */
void SpatterBenchGenerator::tokenizeArgs(const std::string &args, const int32_t &argc, char ***argv)
{
    std::istringstream iss(args);
    std::string tok;

    char **argvPtr = new char *[argc + 1];
    int argvIdx = 0;

    while (iss >> tok) {
        int arg_size = tok.size() + 1;

        argvPtr[argvIdx] = new char[arg_size];
        strncpy(argvPtr[argvIdx], tok.c_str(), arg_size);

        ++argvIdx;
    }
    argvPtr[argvIdx] = nullptr;

    *argv = argvPtr;
}

/**
   * @brief Calculate the number of bytes read or written by memory requests.
   *
   * @param config Run-configuration used to determine the kernel type.
   * @return Number of bytes read or written by memory requests.
   */
uint64_t SpatterBenchGenerator::calcBytes(const Spatter::ConfigurationBase *config)
{
    uint64_t numBytes = 0;

    if ((0 == config->kernel.compare("gather")) || (0 == config->kernel.compare("multigather"))) {
        numBytes = statReadBytes->getCollectionCount() * reqLength;
    } else if ((0 == config->kernel.compare("scatter")) || (0 == config->kernel.compare("multiscatter"))) {
        numBytes = statWriteBytes->getCollectionCount() * reqLength;
    } else if (0 == config->kernel.compare("sg")) {
        numBytes = (statWriteBytes->getCollectionCount() + statReadBytes->getCollectionCount()) * reqLength;
    }

    return numBytes;
}

/**
   * @brief Return the number of elements in the pattern.
   *
   * @param config Run-configuration used to determine the kernel type.
   * @return Number of elements in the pattern.
   */
size_t SpatterBenchGenerator::getPatternSize(const Spatter::ConfigurationBase *config)
{
    size_t patternSize = 0;

    if ((0 == config->kernel.compare("gather")) || (0 == config->kernel.compare("scatter"))) {
        patternSize = config->pattern.size();
    } else if ((0 == config->kernel.compare("sg"))) {
        assert(config->pattern_scatter.size() == config->pattern_gather.size());
        patternSize = config->pattern_scatter.size();
    } else if (0 == config->kernel.compare("multigather")) {
        patternSize = config->pattern_gather.size();
    } else if (0 == config->kernel.compare("multiscatter")) {
        patternSize = config->pattern_scatter.size();
    }

    return patternSize;
}

/**
   * @brief Update the pattern, count, and config indices.
   *
   */
void SpatterBenchGenerator::updateIndices()
{
    const Spatter::ConfigurationBase *config = configList.configs[configIdx].get();
    size_t patternSize = getPatternSize(config);

    if (patternIdx == (patternSize - 1)) {
        patternIdx = 0;
        if (countIdx == (config->count - 1)) {
            countIdx = 0;
            if (!warmupFin) {
                // Check if the warm-up runs for the current run-configuration have completed.
                if (warmupIdx == (warmupRuns - 1)) {
                    warmupFin = true;
                } else {
                    ++warmupIdx;
                }
            }
            if (warmupFin) {
                // Finished issuing requests for the current run-configuration.
                configFin = true;
                ++configIdx;
            }
        } else {
            ++countIdx;
        }
    } else {
        ++patternIdx;
    }
}

/**
   * @brief Output the statistics for the previous Spatter pattern.
   *
   */
void SpatterBenchGenerator::outputStats()
{
    const Spatter::ConfigurationBase *config = configList.configs[configIdx-1].get();
    uint64_t statBytes = calcBytes(config);

    double latencySeconds;
    double bandwidth;
    double timePerCycle;

    // Convert the request latency from nanoseconds to seconds.
    latencySeconds = statReqLatency->getCollectionCount() / 1'000'000'000.0;

    // Convert the recorded bytes to megabytes before calculating the bandwidth.
    bandwidth = (statBytes / 1'000'000.0) / latencySeconds;

    timePerCycle = latencySeconds / statCycles->getCollectionCount();

    out->output("%-15lu",  config->id);
    out->output("%-15lu",  statBytes);
    out->output("%-15g",   latencySeconds);
    out->output("%-15.2f", bandwidth);
    out->output("%-15lu",  statCycles->getCollectionCount());
    out->output("%-15g\n", timePerCycle);
}

/**
   * @brief Generate a memory request for a Gather pattern.
   *
   */
void SpatterBenchGenerator::gather()
{
    const Spatter::ConfigurationBase *config = configList.configs[configIdx].get();

    // The source buffer (sparse) ranges from 0 to (sparse.size() - 1).
    uint64_t startSrc  = 0;
    uint64_t srcOffset = config->pattern[patternIdx] + config->delta * countIdx;

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", startSrc + srcOffset);
    queue->push_back(new MemoryOpRequest((startSrc + srcOffset), reqLength, READ));
}

/**
   * @brief Generate a memory request for a Scatter pattern.
   *
   */
void SpatterBenchGenerator::scatter()
{
    const Spatter::ConfigurationBase *config = configList.configs[configIdx].get();

    // The destination (sparse) buffer ranges from sparse.size() to (dense.size() - 1).
    uint64_t startDst  = config->sparse.size();
    uint64_t dstOffset = config->pattern[patternIdx] + config->delta * countIdx;

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", startDst + dstOffset);
    queue->push_back(new MemoryOpRequest(startDst + dstOffset, reqLength, WRITE));
}

/**
   * @brief Generate memory requests for a GS pattern.
   *
   */
void SpatterBenchGenerator::scatterGather()
{
    const Spatter::ConfigurationBase *config = configList.configs[configIdx].get();

    // The source (sparse_gather) buffer ranges from 0 to (sparse_gather.size() - 1).
    uint64_t startSrc  = 0;
    uint64_t srcOffset = config->pattern_gather[patternIdx] + config->delta_gather * countIdx;

    // The destination (sparse_scatter) buffer ranges from sparse_gather.size() to (sparse_scatter.size() - 1).
    uint64_t startDst  = config->sparse_gather.size();
    uint64_t dstOffset = config->pattern_scatter[patternIdx] + config->delta_scatter * countIdx;

    MemoryOpRequest* readReq  = new MemoryOpRequest(startSrc + srcOffset, reqLength, READ);
    MemoryOpRequest* writeReq = new MemoryOpRequest(startDst + dstOffset, reqLength, WRITE);

    writeReq->addDependency(readReq->getRequestID());

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", startSrc + srcOffset);
    queue->push_back(readReq);

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", startDst + dstOffset);
    queue->push_back(writeReq);
}

/**
   * @brief Generate a memory request for a MultiGather pattern.
   *
   */
void SpatterBenchGenerator::multiGather()
{
    const Spatter::ConfigurationBase *config = configList.configs[configIdx].get();

    // The source (sparse) buffer ranges from 0 to (sparse.size() - 1).
    uint64_t startSrc  = 0;
    uint64_t srcOffset = config->pattern[config->pattern_gather[patternIdx]] + config->delta * countIdx;

    out->verbose(CALL_INFO, 8, 0, "Issuing READ request for address %" PRIu64 "\n", startSrc + srcOffset);
    queue->push_back(new MemoryOpRequest(startSrc + srcOffset, reqLength, READ));
}

/**
   * @brief Generate a memory request for a MultiScatter pattern.
   *
   */
void SpatterBenchGenerator::multiScatter()
{
    const Spatter::ConfigurationBase *config = configList.configs[configIdx].get();

    // The destination (sparse) buffer ranges from dense.size() to (sparse.size() - 1).
    uint64_t startDst  = config->dense.size();
    uint64_t dstOffset = config->pattern[config->pattern_scatter[patternIdx]] + config->delta * countIdx;

    out->verbose(CALL_INFO, 8, 0, "Issuing WRITE request for address %" PRIu64 "\n", startDst + dstOffset);
    queue->push_back(new MemoryOpRequest(startDst + dstOffset, reqLength, WRITE));
}
