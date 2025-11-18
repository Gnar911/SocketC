cmake_minimum_required(VERSION 3.5)

project(IPC LANGUAGES C)

add_executable(IPCpipe IPC_pipe.c)
add_executable(IPCMsgQueue IPC_MsgQueue.c)
