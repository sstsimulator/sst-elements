// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_SYSCALL_DEFINES
#define _H_VANADIS_SYSCALL_DEFINES

/* Syscall related defines that are typically found in linux header files.
 * Replicated here with the name "VANADIS_LINUX_" for use inside vanadis.
 * Some defines that are related to a specific syscall also include the
 * syscall name (e.g., VANADIS_LINUX_CLONE_FILES instead of VANADIS_LINUX_FILES).
*/

// From sched.h, for clone and clone3
#define VANADIS_LINUX_SIGCHLD       17
#define VANADIS_LINUX_CSIGNAL       0x000000ff /* Signal mask to be sent at exit.  */
#define VANADIS_LINUX_CLONE_VM      0x00000100 /* Set if VM shared between processes.  */
#define VANADIS_LINUX_CLONE_FS      0x00000200 /* Set if fs info shared between processes.  */
#define VANADIS_LINUX_CLONE_FILES   0x00000400 /* Set if open files shared between processes.  */
#define VANADIS_LINUX_CLONE_SIGHAND 0x00000800 /* Set if signal handlers shared.  */
#define VANADIS_LINUX_CLONE_PIDFD   0x00001000 /* Set if pidfd should be set in parent. */
#define VANADIS_LINUX_CLONE_PTRACE  0x00002000 /* Set to continue trace on child too. */
#define VANADIS_LINUX_CLONE_VFORK   0x00004000 /* Set to have child wake parent on mm_release */
#define VANADIS_LINUX_CLONE_PARENT  0x00008000 /* Set to have same parent as cloner */
#define VANADIS_LINUX_CLONE_THREAD  0x00010000 /* Set to add to same thread group.  */
#define VANADIS_LINUX_CLONE_NEWNS   0x00020000 /* Set to create new namespace.  */
#define VANADIS_LINUX_CLONE_SYSVSEM 0x00040000 /* Set to shared SVID SEM_UNDO semantics.  */
#define VANADIS_LINUX_CLONE_SETTLS  0x00080000 /* Set TLS info.  */
#define VANADIS_LINUX_CLONE_PARENT_SETTID  0x00100000 /* Store TID in userlevel buffer before MM copy.  */
#define VANADIS_LINUX_CLONE_CHILD_CLEARTID 0x00200000 /* Register exit futex and memory location to clear.  */
#define VANADIS_LINUX_CLONE_DETACHED       0x00400000 /* Create clone detached.  */
#define VANADIS_LINUX_CLONE_CHILD_SETTID   0x01000000 /* Store TID in userlevel buffer in the child.  */
#define VANADIS_LINUX_CLONE_NEWCGROUP      0x02000000  /* New cgroup namespace */
#define VANADIS_LINUX_CLONE_NEWUTS         0x04000000  /* New utsname namespace */
#define VANADIS_LINUX_CLONE_NEWIPC         0x08000000  /* New ipc namespace */
#define VANADIS_LINUX_CLONE_NEWUSER        0x10000000  /* New user namespace */
#define VANADIS_LINUX_CLONE_NEWPID         0x20000000  /* New pid namespace */
#define VANADIS_LINUX_CLONE_NEWNET         0x40000000  /* New network namespace */
#define VANADIS_LINUX_CLONE_IO             0x80000000  /* Clone io context */
#define VANADIS_LINUX_CLONE_CLEAR_SIGHAND  0x100000000ULL /* Clear any signal handler and reset to SIG_DFL. */
#define VANADIS_LINUX_CLONE_INTO_CGROUP    0x200000000ULL /* Clone into a specific cgroup given the right permissions. */
#define VANADIS_LINUX_CLONE_NEWTIME        0x00000080     /* New time namespace */

#endif
