// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_RISCV64_GDB
#define _H_VANADIS_RISCV64_GDB

#include "decoder/vdecoder.h"
#include "inst/vinstall.h"
#include <sst/core/subcomponent.h>

extern "C" {
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
}

/* arbitrary buffer length of 32KB */
#define BUFFER_LEN 0x8000

namespace SST {
namespace Vanadis {

/*
 * This Class implements a GDB server for an individual vanadis hardware thread.
 *
 * This implementation is considered to be in extreme beta. Something is probably
 * going to be wrong.
 *
 * We currently implement this all for RISCV 64 bit, but most of the code can
 * probably be moved to a generic file for both MIPS and RISCV when we get time.
 *
 * We do not use GDB's internal threads, each hardware thread must set its own
 * unique local port, and connect to the server with its own GDB client.
 *
 * We do not halt the simulation on breakpoints. If SST is slowing down your
 * system, it will continue to slow down your system at the breakpoint.
 * This is so other cores and the memory system can continue to make progress.
 *
 * In the same vein, certain gdb commands may take longer than expected as vanadis
 * may be waiting for memory accesses that miss in the caches, try to give commands
 * 20-30 seconds before you assume something went wrong.
 *
 * This is only meant for architectural debugging. You need to use gdb on SST
 * or print statements to debug the micro-arch.
 *
 * You CANNOT load binaries or change code through the GDB client!!!!!
 * Vanadis currently has no working FENCE.I instruction.
 *
 * Tested on gdb-multiarch for ubuntu 20.04 and
 *           riscv-none-embed-gdb from xpack-riscv-none-embed-gcc-10.2.0-1.2-linux-x64
 *
 * Example: May differ based on different gdb clients
 *      gdb-multiarch
 *      set architecture riscv:rv64
 *      file path_to_elf_file_with_debug_symbols
 *      ### Do this to allow commands extra time to return
 *      set remotetimeout 1000
 *      ### change 0x10000000 to be low memory boundary your program uses
 *      ### change 0x10000000 to be high memory boundary your program uses
 *      mem 0x10000000 0x20000000 rw
 *      target remote 127.0.0.1:port_number_you_configured
 *      You should now see GDB interface that you know and love
 * */
class VanadisRISCV64GDB : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisRISCV64GDB)
    SST_ELI_REGISTER_SUBCOMPONENT(
      VanadisRISCV64GDB, "vanadis", "VanadisRISCV64GDB",
      SST_ELI_ELEMENT_VERSION(1, 0, 0),
      "Implements a RISCV64-compatible GDB server compatible for running GDB client on a single Vanadis hardware thread.",
      SST::Vanadis::VanadisRISCV64GDB)

    SST_ELI_DOCUMENT_PARAMS({ "gdb_active", "Should this hardware thread use GDB" },
                            { "gdb_port", "What local host port number should this thread's GDB server listen on." },
                            { "gdb_break_at_start", "Should GDB server break on the first instruction of the decoder." },
                            { "gdb_initial_break_addr", "Address of extra breakpoint to add at start. 0x0 == no break." },
                            { "gdb_debug_comms", "Print input and output traffic between client and server." },
                            { "gdb_break_limit", "How many total breakpoints are supported for this thread." })

    VanadisRISCV64GDB(ComponentId_t id, Params& params) : SubComponent(id) {
        active = params.find<bool>("gdb_active", false);
        port = params.find<uint16_t>("gdb_port", 0);
        break_at_start = params.find<bool>("gdb_break_at_start", false);
        break_limit = params.find<uint16_t>("gdb_break_limit", 50);
        initial_break_addr = params.find<uint64_t>("gdb_initial_break_addr", 0);
        debug_comms = params.find<bool>("gdb_debug_comms", false);

        if ( initial_break_addr != 0 ) {
            insert_breakpoint( initial_break_addr );
        }

        if ( break_at_start ) {
            interrupt = true;
        }

        state_m = &poll_for_packet_stub;
    }

    virtual ~VanadisRISCV64GDB() {
        breakpoints.clear();
    }

    bool is_bp_at( uint64_t address ) {
        return (breakpoints.find(address) != breakpoints.end());
    }

    void remove_breakpoint( uint64_t address ) {
        if ( is_bp_at(address) ) {
            breakpoints.erase(address);
        }
    }

    void insert_breakpoint( uint64_t address ) {
        if ( is_bp_at(address) ) {
            return;
        }

        if ( breakpoints.size() >= break_limit ) {
            output->fatal(CALL_INFO, -1, "Error - Trying to insert breakpoint at: 0x%llx exceeds limit of %d.\n", address, break_limit);
        }

        breakpoints.insert(std::pair<uint64_t, bool>(address, true));
    }

    /*
     * Called by the decoder before it inserts an instruction into the back-end
     *
     * Detects breakpoints that are reached in running software
     * */
    bool should_stall( uint64_t address ) {
        if ( active ) {
            if ( in_break && !pass_1 ) {
                // Always stall inside a breakpoint until pass_1 is set
                return true;
            }

            bool wait_for_empty = false;

            if ( pass_1 && current_break_addr != address ) {
                // In a breakpoint and we want to step to the next instruction
                //   and we have found a new address
                wait_for_empty = true;
            }

            if ( !in_break && is_bp_at(address) ) {
                // We are not in a breakpoint and the next instruction
                //   matches a breakpoint address
                wait_for_empty = true;
            }

            if ( interrupt ) {
                // We got an interrupt from the client
                //   flush the pipeline so we can break somewhere
                wait_for_empty = true;
            }

            if ( wait_for_empty ) {
                if ( !decoder->is_rob_empty() ) {
                    // Hold back instructions until pipeline is empty
                    //   There could be a jump in the pipeline
                    wait_for_squash_count = 0;
                    return true;
                }

                if ( wait_for_squash_count < 20 ) {
                    // The ROB is empty
                    //   The last instruction to retire could have been a jump
                    //   Wait for the decoder to have a chance to internally
                    //   redirect to the correct IP
                    wait_for_squash_count++;
                    return true;
                }

                // Done waiting, ip should be real next instruction address
                wait_for_squash_count = 0;

                if ( is_bp_at(address) || pass_from_step || interrupt ) {
                    pass_1 = false;
                    in_break = true;
                    current_break_addr = address;
                    // tell the client we found the next breakpoint
                    send_packet("S05");
                    output->verbose(CALL_INFO, 0, 0, "Stopping at address |0x%llx| for breakpoint.\n", current_break_addr);
                    if ( interrupt ) {
                        output->verbose(CALL_INFO, 0, 0, "Stop caused by interrupt.\n");
                        interrupt = false;
                    }
                    if ( pass_from_step ) {
                        output->verbose(CALL_INFO, 0, 0, "Stop caused by stepping.\n");
                        pass_from_step = false;
                    }
                    return true;
                }
            }
        }

        // Keep inserting instructions into the pipeline
        return false;
    }

    /*
     * Called from pipeline every cycle to tick our time
     * */
    void tick() {
        if ( active ) {
            if ( in_break ) {
                // In a breakpoint, talk to the client
                (*state_m)(this);
            }
            else {
                // Not in a breakpoint, but check if the client
                // wants us to stop somewhere
                if ( poll_for_interrupt() ) {
                    interrupt = true;
                }
            }
        }
    }

    /*
     * We got a valid packet from the client
     *  service the request
     * */
    void do_packet() {
        char type = pkt_buf[0];
        char * payload = (char *)&pkt_buf[1];

        switch (type) {
            case 'c': {
                    // Continue command
                    if ( strlen(payload) != 0 ) {
                        // We do not support fancy continues
                        send_packet("E95");
                    }
                    else {
                        pass_1 = true;
                        pass_from_step = false;
                    }
                    state_m = &poll_for_packet_stub;
                }
                break;
            case 'g': {
                    // Dump all GP registers
                    read_reg_index = -1;

                    state_m = &read_all_reg_stub;

                    data_buf_len = 0;

                    read_all_reg();
                }
                break;
            case 'm': {
                    // Read memory
                    data_buf_len = 0;

                    int args_found = sscanf(payload, "%lx,%lx", &mem_addr, &mem_len);

                    if ( args_found == 2 ) {
                        if ((mem_len * 2) + 1 <= BUFFER_LEN && mem_len >= 1 ) {
                            state_m = &read_mem_bytes_stub;
                            mem_first_byte = true;

                            read_mem_bytes();
                            break;
                        }
                    }

                    output->verbose(CALL_INFO, 0, 0, "Invalid memory packet from client\n");
                    send_packet("E98");
                    state_m = &poll_for_packet_stub;
                    break;
                }
                break;
            case 'M': {
                    // Write memory
                    data_buf_len = 0;

                    int args_found = sscanf(payload, "%lx,%lx", &mem_addr, &mem_len);
                    char * hex_coded = strchr(payload, ':') + 1;

                    if ( args_found == 2 && hex_coded != NULL ) {
                        if ((mem_len * 2) + 1 <= BUFFER_LEN && mem_len >= 1 ) {
                            state_m = &write_mem_bytes_stub;
                            mem_first_byte = true;
                            mem_index = 0;

                            read_hex_to_databuff( (uint8_t *)hex_coded, mem_len );
                            break;
                        }
                    }

                    output->verbose(CALL_INFO, 0, 0, "Invalid memory write packet from client\n");
                    send_packet("E94");
                    state_m = &poll_for_packet_stub;
                    break;
                }
                break;
            case 'p': {
                    // Read register, register #32 is the program counter
                    int i = strtol(payload, NULL, 16);
                    if ( i >= 32 && i != 32 )
                    {
                        send_packet("E01");
                        state_m = &poll_for_packet_stub;
                        break;
                    }

                    read_reg_index = i;

                    state_m = &read_one_reg_stub;

                    data_buf_len = 0;

                    read_one_reg();
                }
                break;
            case 'P': {
                    // Write register, program counter is not allowed, sorry
                    char * equals = strchr(payload, '=');
                    *equals = '\0';
                    int i = strtol(payload, NULL, 16);
                    char * hex_coded = equals + 1;
                    if ( (i >= 32 ) || hex_coded == NULL)
                    {
                        send_packet("E01");
                        state_m = &poll_for_packet_stub;
                        break;
                    }

                    data_buf_len = 0;

                    read_hex_to_databuff( (uint8_t *)hex_coded, 8 );

                    if ( state_m == &poll_for_packet_stub ) {
                        break;
                    }

                    write_reg_value = *(uint64_t *)data_buf;

                    write_reg_index = i;

                    write_reg_ret = &poll_for_packet_stub;

                    state_m = &write_reg_A_stub;

                    send_packet("OK");
                }
                break;
            case 'q': {
                    // Query packet, we don't support much
                    //   While we technically supprt software breakpoints we prefer hardware
                    //   Tell the client we don't do software, but they don't seem to care
                    if ( !strncmp( payload, "Supported:", strlen("Supported:") ) ) {
                        send_packet("PacketSize=8000;qXfer:features:read-;multiprocess-;swbreak-;hwbreak+;vContSupported-");
                    }
                    else {
                        send_packet("");
                    }
                    state_m = &poll_for_packet_stub;
                }
                break;
            case 's': {
                    // Step command, no fancy arguments allowed
                    if ( strlen(payload) != 0 ) {
                        send_packet("E99");
                    }
                    else {
                        pass_1 = true;
                        pass_from_step = true;
                    }
                    state_m = &poll_for_packet_stub;
                }
                break;
            case 'X': {
                    // Write memory in binary
                    data_buf_len = 0;

                    int args_found = sscanf(payload, "%lx,%lx", &mem_addr, &mem_len);
                    char * bin_coded = strchr(payload, ':') + 1;

                    if ( args_found == 2 && bin_coded != NULL ) {
                        if (mem_len < BUFFER_LEN && mem_len >= 1 ) {
                            state_m = &write_mem_bytes_stub;
                            mem_first_byte = true;
                            mem_index = 0;

                            int skipped_bytes = (bin_coded - payload) + 1;

                            read_bin_to_databuff( (uint8_t *)bin_coded, pkt_buf_len - skipped_bytes );
                            if ( mem_len != data_buf_len ) {
                                output->verbose(CALL_INFO, 0, 0, "Invalid memory write packet length after filter from client\n");
                                send_packet("E91");
                                state_m = &poll_for_packet_stub;
                                break;
                            }
                            break;
                        }
                    }

                    output->verbose(CALL_INFO, 0, 0, "Invalid memory write packet from client\n");
                    send_packet("E92");
                    state_m = &poll_for_packet_stub;
                    break;
                }
                break;
            case 'Z': {
                    // Insert breakpoint
                    //   We accept both software and hardware, but we treat them the same
                    uint64_t type, addr, length;
                    int args_found = sscanf(payload, "%lx,%lx,%lx", &type, &addr, &length);

                    if ( args_found == 3 ) {
                        if ( (type == 1 || type == 0) && (length == 2 || length == 4) ) {
                            insert_breakpoint( addr );
                            state_m = &poll_for_packet_stub;
                            send_packet("OK");
                            break;
                        }
                    }

                    output->verbose(CALL_INFO, 0, 0, "Invalid insert breakpoint packet\n");
                    send_packet("E97");
                    state_m = &poll_for_packet_stub;
                }
                break;
            case 'z': {
                    // Remove breakpoint
                    //   We accept both software and hardware, but we treat them the same
                    uint64_t type, addr, length;
                    int args_found = sscanf(payload, "%lx,%lx,%lx", &type, &addr, &length);

                    if ( args_found == 3 ) {
                        if ( (type == 0 || type == 1) && (length == 2 || length == 4) ) {
                            remove_breakpoint( addr );
                            state_m = &poll_for_packet_stub;
                            send_packet("OK");
                            break;
                        }
                    }

                    output->verbose(CALL_INFO, 0, 0, "Invalid remove breakpoint packet\n");
                    send_packet("E96");
                    state_m = &poll_for_packet_stub;
                }
                break;
            case '?':
                // Get break reason, always send back we were interrupted
                send_packet("S05");
                state_m = &poll_for_packet_stub;
                break;
            default:
                // We don't support the packet, respond empty
                send_packet("");
                state_m = &poll_for_packet_stub;
                break;
        }
    }

    /*
     * Process for writing a memory byte
     *  ## Save user value of register x1
     *  write_mem_byte_A => read_reg_A => read_reg_B => write_mem_byte_B
     *
     *  ## Write input value into register x1
     *  write_mem_byte_B => write_reg_A => write_reg_B => write_mem_byte_C
     *
     *  ## Inject sb x1 (x0 + addr) into the pipeline
     *  write_mem_byte_C => write_mem_byte_D
     *
     *  ## Re-write saved user x1 value to x1
     *  write_mem_byte_D => write_reg_A => write_reg_B => return
     * */
    static void write_mem_byte_A_stub( VanadisRISCV64GDB * me ) {
        me->write_mem_byte_A();
    }

    /*
     * Save user value of register x1
     * */
    void write_mem_byte_A() {
        read_reg_index = 1;

        read_reg_ret = &write_mem_byte_B_stub;

        state_m = &read_reg_A_stub;

        return;
    }

    static void write_mem_byte_B_stub( VanadisRISCV64GDB * me ) {
        me->write_mem_byte_B();
    }

    /*
     * Write input value into register x1
     * */
    void write_mem_byte_B() {
        mem_saved_reg = read_reg_value;

        write_reg_value = data_buf[mem_index];

        write_reg_index = 1;

        write_reg_ret = write_mem_byte_C_stub;

        state_m = &write_reg_A_stub;

        return;
    }

    static void write_mem_byte_C_stub( VanadisRISCV64GDB * me ) {
        me->write_mem_byte_C();
    }

    /*
     * Inject sb x1 (x0 + addr) into the pipeline
     * */
    void write_mem_byte_C() {
        if ( decoder->is_rob_empty() ) {
            VanadisInstruction* next_ins = new VanadisStoreInstruction(
                                                current_break_addr, decoder->getHardwareThread(),
                                                decoder->getDecoderOptions(), 0, mem_addr, 1, 1,
                                                MEM_TRANSACTION_NONE, STORE_INT_REGISTER);

            decoder->instruction_to_rob( output, next_ins, NULL, NULL );

            state_m = &write_mem_byte_D_stub;
        }

        return;
    }

    static void write_mem_byte_D_stub( VanadisRISCV64GDB * me ) {
        me->write_mem_byte_D();
    }

    /*
     * Re-write saved user x1 value to x1
     * */
    void write_mem_byte_D() {
        write_reg_value = mem_saved_reg;

        write_reg_index = 1;

        write_reg_ret = write_mem_byte_ret;

        state_m = &write_reg_A_stub;

        return;
    }

    static void write_mem_bytes_stub( VanadisRISCV64GDB * me ) {
        me->write_mem_bytes();
    }

    /*
     * Write to memory 1 byte at a time
     *
     * Respond with the OK packet once done
     *
     * Vanadis will just die if something goes wrong
     * */
    void write_mem_bytes() {
        if ( !mem_first_byte ) {
            mem_len--;
            mem_addr++;
            mem_index++;
        }

        if ( mem_len <= 0 ) {
            send_packet("OK");;

            state_m = &poll_for_packet_stub;
        }
        else {
            mem_first_byte = false;
            write_mem_byte_ret = &write_mem_bytes_stub;
            state_m = &write_mem_byte_A_stub;
        }

        return;
    }

    /*
     * Process for reading a memory byte
     *  ## Save user value of register x1
     *  read_mem_byte_A => read_reg_A => read_reg_B => read_mem_byte_B
     *
     *  ## Inject lb x1 (x0 + addr) into the pipeline
     *  read_mem_byte_B => read_mem_byte_C
     *
     *  ## Get the read in byte from x1
     *  read_mem_byte_C => read_reg_A => read_reg_B => read_mem_byte_D
     *
     *  ## Re-write saved user x1 value to x1
     *  read_mem_byte_D => write_reg_A => write_reg_B => return
     * */
    static void read_mem_byte_A_stub( VanadisRISCV64GDB * me ) {
        me->read_mem_byte_A();
    }

    /*
     * Save user value of register x1
     * */
    void read_mem_byte_A() {
        read_reg_index = 1;

        read_reg_ret = &read_mem_byte_B_stub;

        state_m = &read_reg_A_stub;

        return;
    }

    static void read_mem_byte_B_stub( VanadisRISCV64GDB * me ) {
        me->read_mem_byte_B();
    }

    /*
     * Inject lb x1 (x0 + addr) into the pipeline
     * */
    void read_mem_byte_B() {
        mem_saved_reg = read_reg_value;

        if ( decoder->is_rob_empty() ) {
            VanadisInstruction* next_ins = new VanadisLoadInstruction(
                                                current_break_addr, decoder->getHardwareThread(),
                                                decoder->getDecoderOptions(), 0, mem_addr, 1, 1,
                                                false, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER);

            decoder->instruction_to_rob( output, next_ins, NULL, NULL );

            state_m = &read_mem_byte_C_stub;
        }

        return;
    }

    static void read_mem_byte_C_stub( VanadisRISCV64GDB * me ) {
        me->read_mem_byte_C();
    }

    /*
     * Get the read in byte from x1
     * */
    void read_mem_byte_C() {
        if ( decoder->is_rob_empty() ) {
            read_reg_index = 1;

            read_reg_ret = &read_mem_byte_D_stub;

            state_m = &read_reg_A_stub;
        }

        return;
    }

    static void read_mem_byte_D_stub( VanadisRISCV64GDB * me ) {
        me->read_mem_byte_D();
    }

    /*
     * Re-write saved user x1 value to x1
     * */
    void read_mem_byte_D() {
        mem_data = read_reg_value & 0xff;

        write_reg_value = mem_saved_reg;

        write_reg_index = 1;

        write_reg_ret = read_mem_byte_ret;

        state_m = &write_reg_A_stub;

        return;
    }

    static void read_mem_bytes_stub( VanadisRISCV64GDB * me ) {
        me->read_mem_bytes();
    }

    /*
     * Read memory 1 byte at a time into the data_buf
     *  In hex format
     *
     * Respond with the hex data as a packet once done
     *
     * Vanadis will just die if something goes wrong
     * */
    void read_mem_bytes() {
        if ( !mem_first_byte ) {
            mem_len--;
            mem_addr++;
            write_to_databuff_as_hex((uint8_t *)&mem_data, 1);
        }

        if ( mem_len <= 0 ) {
            send_packet((const char*)data_buf);

            state_m = &poll_for_packet_stub;
        }
        else {
            mem_first_byte = false;
            read_mem_byte_ret = &read_mem_bytes_stub;
            state_m = &read_mem_byte_A_stub;
        }

        return;
    }

    static void write_reg_A_stub( VanadisRISCV64GDB * me ) {
        me->write_reg_A();
    }

    /*
     * Insert instruction into pipeline
     *  write_reg_call() will be called when instruction executes
     * */
    void write_reg_A() {
        if ( decoder->is_rob_empty() ) {
            VanadisInstruction* next_ins = new VanadisSetRegisterByCallInstruction<int64_t>(
                                                    current_break_addr, decoder->getHardwareThread(),
                                                    decoder->getDecoderOptions(), write_reg_index,
                                                    &write_reg_call_stub, (void *)this );

            decoder->instruction_to_rob( output, next_ins, NULL, NULL );

            write_reg_done = false;

            state_m = &write_reg_B_stub;
        }

        return;
    }

    static void * write_reg_call_stub( void * arg_input ) {
        VanadisRISCV64GDB * me = (VanadisRISCV64GDB *)arg_input;
        return me->write_reg_call( );
    }

    /*
     * Called by executing instruction
     *  The value we return is written to the architectural register
     * */
    void * write_reg_call( ) {
        write_reg_done = true;

        return (void *)&write_reg_value;
    }

    static void write_reg_B_stub( VanadisRISCV64GDB * me ) {
        me->write_reg_B();
    }

    /*
     * We hold until the value actually wrote to the register
     * */
    void write_reg_B() {
        if ( write_reg_done ) {
            state_m = write_reg_ret;
        }

        return;
    }

    static void read_reg_A_stub( VanadisRISCV64GDB * me ) {
        me->read_reg_A();
    }

    /*
     * Insert instruction into pipeline
     *  read_reg_call() will be called when instruction executes
     * */
    void read_reg_A() {
        if ( decoder->is_rob_empty() ) {
            VanadisInstruction* next_ins = new VanadisGetRegisterByCallInstruction<int64_t>(
                                                    current_break_addr, decoder->getHardwareThread(),
                                                    decoder->getDecoderOptions(), read_reg_index,
                                                    &read_reg_call_stub, (void *)this );

            decoder->instruction_to_rob( output, next_ins, NULL, NULL );

            read_reg_done = false;

            state_m = &read_reg_B_stub;
        }

        return;
    }

    static void read_reg_call_stub( void * arg_input, void * arg_ret ) {
        VanadisRISCV64GDB * me = (VanadisRISCV64GDB *)arg_input;
        int64_t value = *(int64_t *)(arg_ret);
        me->read_reg_call( value );
    }

    /*
     * Called by executing instruction
     *  The value we get came from the read register
     * */
    void read_reg_call( int64_t value ) {
        read_reg_value = value;

        read_reg_done = true;
    }

    static void read_reg_B_stub( VanadisRISCV64GDB * me ) {
        me->read_reg_B();
    }

    /*
     * We hold until the value actually read from the register
     * */
    void read_reg_B() {
        if ( read_reg_done ) {
            state_m = read_reg_ret;
        }

        return;
    }

    static void read_all_reg_stub( VanadisRISCV64GDB * me ) {
        me->read_all_reg();
    }

    /*
     * Read all integer registers 1 register at a time
     *
     * Write the outputs as hex to the data_buf
     *
     * Send out the hex data as a packet when done
     * */
    void read_all_reg() {
        if ( read_reg_index >= 0 ) {
            write_to_databuff_as_hex((uint8_t *)&read_reg_value, 8);
        }

        if ( read_reg_index >= 31 ) {
            send_packet((const char*)data_buf);

            state_m = &poll_for_packet_stub;
        }
        else {
            read_reg_index++;

            read_reg_ret = &read_all_reg_stub;

            state_m = &read_reg_A_stub;
        }

        return;
    }

    static void read_one_reg_stub( VanadisRISCV64GDB * me ) {
        me->read_one_reg();
    }

    /*
     * Read 1 integer register
     *
     * Register index 32 is the program counter
     *
     * Process if normal integer register
     *
     * read_one_reg => read_reg_A => read_reg_B => read_one_reg_end => done
     *
     * */
    void read_one_reg() {
        if ( read_reg_index == 32 ) {
            write_to_databuff_as_hex((uint8_t *)&current_break_addr, 8);

            send_packet((const char*)data_buf);

            state_m = &poll_for_packet_stub;
        }
        else {
            read_reg_ret = &read_one_reg_end_stub;

            state_m = &read_reg_A_stub;
        }

        return;
    }

    static void read_one_reg_end_stub( VanadisRISCV64GDB * me ) {
        me->read_one_reg_end();
    }

    void read_one_reg_end() {
        write_to_databuff_as_hex((uint8_t *)&read_reg_value, 8);

        send_packet((const char*)data_buf);

        state_m = &poll_for_packet_stub;

        return;
    }

    /*
     * If we are not getting data we don't want to poll every
     * cycle of the simulation, as this will slow down the simulation
     * considerably. We poll once every 1000 cycles instead.
     * */
    bool poll_delay_done() {
        return poll_delay_cnt++ < 1000;
    }

    static void poll_for_packet_stub( VanadisRISCV64GDB * me ) {
        me->poll_for_packet();
    }

    /*
     * Read in bytes until we find the '$' start of packet
     * */
    void poll_for_packet() {
        if ( poll_delay_done() ) {
            return;
        }

        char buffer;

        while ( get_char_gdb(&buffer) ) {
            if ( buffer == '$' ) {
                sum = 0;
                state_m = &read_packet_stub;
                pkt_buf_len = 0;
                read_packet();
                return;
            }
            if ( buffer == '#' ) {
                // We might have started reading mid packet
                //   See if they can resend
                send_nack();
            }
        }

        poll_delay_cnt = 0;

        return;
    }

    static void read_packet_stub( VanadisRISCV64GDB * me ) {
        me->read_packet();
    }

    /*
     * Read in the packet and calculate the checksum until
     *  we get the '#' end of packet character
     *
     *  Once we find the end, the next 2 characters are hex
     *   encoded checksum
     * */
    void read_packet( ) {
        if ( poll_delay_done() ) {
            return;
        }

        char buffer;

        while ( get_char_gdb(&buffer) ) {
            if ( buffer == '#' ) {
                pkt_buf[pkt_buf_len] = '\0';
                hex_builder_ret = &check_packet_stub;
                state_m = &read_hex_stub;
                hex_index = 0;
                hex_len = 2;
                read_hex();
                return;
            }

            if ( pkt_buf_len >= sizeof(pkt_buf) ) {
                output->fatal(CALL_INFO, -1, "Error - GDB rec buff overflow.\n");
            }

            pkt_buf[pkt_buf_len++] = buffer;
            sum += (uint8_t)buffer;
        }

        poll_delay_cnt = 0;

        return;
    }

    static void check_packet_stub( VanadisRISCV64GDB * me ) {
        me->check_packet();
    }

    /*
     * Ensure the checksum matched
     *   if it did
     *      Ack back with a '+' and service the packet
     *   if id didn't
     *      Nack back with a '-'
     * */
    void check_packet( ) {
        if ( (uint8_t)hex_builder == sum ) {
            send_ack();
            do_packet();
        }
        else {
            send_nack();
            state_m = &poll_for_packet_stub;
        }

        return;
    }

    /*
     * We need to check if the client wants us to stop
     *   everyonce in a while 0x3 is the classic stop byte
     *   but if the client is trying to send us packets, lets stop anyway
     * */
    bool poll_for_interrupt() {
        if ( poll_delay_done() ) {
            return false;
        }

        char buffer;

        while ( get_char_gdb(&buffer) ) {
            if ( buffer == 0x3 || buffer == '$' ) {
                if ( buffer == 0x3 ) {
                    send_ack();
                }
                return true;
            }
        }

        poll_delay_cnt = 0;

        return false;
    }

    /*
     * Ack '+' and Nack '-'
     * */
    void send_ack() {
        write_to_buff_raw((uint8_t *)"+", 1);
        flush_buff_to_socket();
    }

    void send_nack() {
        write_to_buff_raw((uint8_t *)"-", 1);
        flush_buff_to_socket();
    }

    /*
     * Append raw bytes to the send buffer
     * */
    void write_to_buff_raw( uint8_t *buf, ssize_t len ) {
        if (send_buf_len + len >= sizeof(send_buf)) {
            output->fatal(CALL_INFO, -1, "Error - GDB send buff overflow.\n");
        }
        memcpy(&send_buf[send_buf_len], buf, len);
        send_buf_len += len;
    }

    /*
     * Send the send_buf out over the socket until it all transmits
     *
     * If we are debugging the comms, write bytes directly to standard out
     *   Most of the bytes are displayable characters, but some are binary.
     *   Will need to pipe output to a file and use hexdump to see these, sorry
     *
     * If we are not connected to the client, just drop the packet so we don't hang.
     * */
    void flush_buff_to_socket() {
        int bytes_left = send_buf_len;
        int offset = 0;

        if ( !connected ) {
            return;
        }

        while ( 1 ) {
            int wrote = write(sock_fd, send_buf + offset, bytes_left);

            if ( wrote == 0 ) {
                output->verbose(CALL_INFO, 0, 0, "GDB client closed connection while server trying to respond\n");
                connected = false;
                return;
            }

            if ( wrote > 0 ) {
                if ( debug_comms ) {
                    std::cout << "\nOUT :: ";
                    for ( int i = 0; i < wrote; i++ ) {
                        std::cout << send_buf[offset + i];
                    }
                    std::cout << "\n";
                }
                bytes_left -= wrote;

                if ( bytes_left == 0 ) {
                    break;
                }
            }
        }

        send_buf_len = 0;
    }

    /*
     * Try to open up our port for listening
     * */
    void try_listen() {
        const int one = 1;
        struct sockaddr_in addr;
        int ret;

        listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
        if (listen_fd < 0) {
            output->fatal(CALL_INFO, -1, "Error - Failure trying to open socket for GDB.\n");
        }

        ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (ret < 0) {
            output->fatal(CALL_INFO, -1, "Error - Could not set socket option for GDB socket SO_REUSEADDR.\n");
        }

        output->verbose(CALL_INFO, 0, 0, "GDB listening on port |%d|\n", port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(port);

        if (addr.sin_addr.s_addr == INADDR_NONE) {
            output->fatal(CALL_INFO, -1, "Error - Failure converting IP address for |127.0.0.1|\n");
        }

        ret = bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
        if (ret < 0) {
            output->fatal(CALL_INFO, -1, "Error - bind for GDB failed on |127.0.0.1:%d|\n", port);
        }

        ret = listen(listen_fd, 1);
        if (ret < 0) {
            output->fatal(CALL_INFO, -1, "Error - listen for GDB failed on |127.0.0.1:%d|\n", port);
        }

        listening = true;
    }

    /*
     * See if we can connect to the client
     * */
    void try_connect() {
        if ( !listening ) {
            try_listen();
        }

        fd_set rfds;
        struct timeval tv;
        int ret;
        const int one = 1;

        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 0;

        ret = select(listen_fd + 1, &rfds, NULL, NULL, &tv);

        if (ret < 1) {
            return;
        }

        sock_fd = accept(listen_fd, NULL, NULL);
        if (sock_fd < 0) {
            output->fatal(CALL_INFO, -1, "Error - GDB socket accept failed with data available\n");
        }

        ret = setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
        if (ret < 0) {
            output->fatal(CALL_INFO, -1, "Error - Could not set socket option for GDB socket SO_KEEPALIVE.\n");
        }

        ret = setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        if (ret < 0) {
            output->fatal(CALL_INFO, -1, "Error - Could not set socket option for GDB socket TCP_NODELAY.\n");
        }

        output->verbose(CALL_INFO, 0, 0, "GDB server is connected to client\n");
        connected = true;

        return;
    }

    /*
     * Read a single character from the socket in a non-blocking way
     *   I realize this is slow, but there won't be much traffic
     *   and it won't have crazy latency requirements
     * */
    bool get_char_gdb( char * buffer ) {
        if ( !connected ) {
            try_connect();

            if ( !connected ) {
                return false;
            }
        }

        fd_set rfds;
        struct timeval tv;
        int ret;

        FD_ZERO(&rfds);
        FD_SET(sock_fd, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = 0;

        ret = select(sock_fd + 1, &rfds, NULL, NULL, &tv);

        if (ret < 1) {
            return false;
        }

        ret = read(sock_fd, buffer, 1);
        if (ret == 0) {
            output->verbose(CALL_INFO, 0, 0, "GDB client closed connection, still listening\n");
            connected = false;
            return false;
        }
        else if ( ret == 1 ) {
            if ( debug_comms ) {
                std::cout << buffer[0] << std::flush;
            }
            return true;
        }

        return false;
    }

    /*
     * Decoder calls this to pass us our needed functions
     * */
    void register_pipeline( SST::Output* output_p, VanadisDecoder * dec ) {
        output = output_p;
        decoder = dec;
    }

    /*
     * Write out a proper GDB server packet
     *   Starts with '$' and ends with '#' followed by 2 byte hex checksum
     * */
    void send_packet_w_size(const uint8_t *pkt, size_t size) {
        uint8_t checksum = 0;
        size_t i;
        write_to_buff_raw((uint8_t *)"$", 1);
        for (i = 0; i < size; i++) {
            checksum += pkt[i];
        }
        write_to_buff_raw((uint8_t *)pkt, size);
        write_to_buff_raw((uint8_t *)"#", 1);

        char buf[3];

        snprintf(buf, 3, "%02x", checksum);
        write_to_buff_raw((uint8_t *)buf, 2);

        flush_buff_to_socket();
    }

    void send_packet(const char *pkt) {
        send_packet_w_size((const uint8_t *)pkt, strlen(pkt));
    }

    /*
     * Convert a single hex character to a 4 bit integer
     * */
    bool hex_to_int(char hex, uint8_t * result) {
        if ((hex >= 'a') && (hex <= 'f')) {
            *result = ((hex - 'a') + 10);
            return true;
        }
        else if ((hex >= '0') && (hex <= '9')) {
            *result = (hex - '0');
            return true;
        }
        else if ((hex >= 'A') && (hex <= 'F')) {
            *result = ((hex - 'A') + 10);
            return true;
        }
        return false;
    }

    static void read_hex_stub( VanadisRISCV64GDB * me ) {
        me->read_hex();
    }

    /*
     * Create a number from an N character hex string coming from the socket
     * */
    void read_hex( ) {
        if ( poll_delay_done() ) {
            return;
        }

        uint8_t buffer;
        uint64_t number;

        if ( hex_index == 0 ) {
            hex_builder = 0;
        }

        if ( hex_index >= hex_len ) {
            state_m = hex_builder_ret;
            (*hex_builder_ret)(this);
            return;
        }

        while ( get_char_gdb((char *)&buffer) ) {
            if ( hex_to_int( buffer, (uint8_t *)&number) ) {
                hex_builder += (number << (((hex_len - hex_index) - 1) * 4));
                hex_index++;
                if ( hex_index >= hex_len ) {
                    state_m = hex_builder_ret;
                    (*hex_builder_ret)(this);
                    return;
                }
            }
            else {
                send_nack();
                state_m = &poll_for_packet_stub;
                return;
            }
        }

        poll_delay_cnt = 0;

        return;
    }

    /*
     * Write 2 hex characters for each passed in char to
     *   the data buff and NUL terminate the string
     * */
    void write_to_databuff_as_hex(uint8_t * data, int count) {
        uint8_t byte;
        for (int i = 0; i < count; i++) {
            byte = data[i];
            data_buf[data_buf_len++] = int_to_hex[byte >> 4];
            data_buf[data_buf_len++] = int_to_hex[byte & 0xf];
        }
        data_buf[data_buf_len] = '\0';
    }

    /*
     * Read in X bytes from hex stream into data buff
     *  hex stream will be twice as long as data buff
     * */
    void read_hex_to_databuff(uint8_t * hex, int count) {
        uint8_t high;
        uint8_t low;
        uint8_t byte;
        int i;
        for ( i = 0; i < count; i++ ) {
            hex_index = i * 2;
            if ( !hex_to_int( hex[hex_index], &high) ) {
                send_packet("E93");
                state_m = &poll_for_packet_stub;
                return;
            }
            if ( !hex_to_int( hex[hex_index + 1], &low) ) {
                send_packet("E93");
                state_m = &poll_for_packet_stub;
                return;
            }
            byte = (high << 4) + low;

            data_buf[i] = byte;
        }

        data_buf_len = i;
        data_buf[data_buf_len] = '\0';
    }

    /*
     * Copy binary data to data buff
     *  '}' is used as an escape character
     *  skip this character and do the next character xored with 0x20
     * */
    void read_bin_to_databuff(uint8_t * bin, int count) {
        int i;
        int k;
        for ( i = 0, k = 0; i < count; i++ ) {
            uint8_t byte = bin[i];
            if ( byte == '}') {
                i++;
                byte = bin[i] ^ 0x20;
            }

            data_buf[k++] = byte;
            data_buf_len++;
        }

        data_buf[data_buf_len] = '\0';
    }

    // Convert 4 bit integer to hex character
    const char * int_to_hex = "0123456789abcdef";

    bool active;
    int port;
    bool break_at_start;
    int break_limit;
    uint64_t initial_break_addr;
    bool debug_comms;
    SST::Output* output;
    std::unordered_map<uint64_t, bool> breakpoints;

    uint64_t current_break_addr;
    bool in_break;
    bool pass_1;
    bool pass_from_step;
    bool interrupt;
    int wait_for_squash_count;
    int poll_delay_cnt;

    void (* state_m)( VanadisRISCV64GDB * );

    int listen_fd;
    int sock_fd;
    bool connected;
    bool listening;

    uint8_t pkt_buf[BUFFER_LEN];
    int pkt_buf_len;
    uint8_t send_buf[BUFFER_LEN];
    int send_buf_len;
    uint8_t data_buf[BUFFER_LEN];
    int data_buf_len;
    uint8_t sum;

    VanadisDecoder * decoder;

    uint64_t hex_builder;
    int hex_index;
    int hex_len;
    void (* hex_builder_ret)( VanadisRISCV64GDB * );

    int read_reg_index;
    int64_t read_reg_value;
    bool read_reg_done;
    void (* read_reg_ret)( VanadisRISCV64GDB * );

    int write_reg_index;
    int64_t write_reg_value;
    bool write_reg_done;
    void (* write_reg_ret)( VanadisRISCV64GDB * );

    uint64_t mem_addr;
    uint64_t mem_len;
    uint64_t mem_index;
    int64_t mem_data;
    int64_t mem_saved_reg;
    bool mem_first_byte;
    void (* read_mem_byte_ret)( VanadisRISCV64GDB * );
    void (* write_mem_byte_ret)( VanadisRISCV64GDB * );
};
} // namespace Vanadis
} // namespace SST

#endif
