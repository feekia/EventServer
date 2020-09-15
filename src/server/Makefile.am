AUTOMAKE_OPTIONS= foreign  
CXXFLAGS += -pg -Wall  -I ../../include -std=gnu++14
bin_PROGRAMS= EventServer
EventServer_SOURCES= acceptor.cpp socketholder.cpp channel.cpp buffer.cpp Handler.cpp Message.cpp threadpools.cpp EventServer.cpp
EventServer_LDFLAGS = 
LIBS +=-levent -levent_pthreads -levent_core -lpthread
