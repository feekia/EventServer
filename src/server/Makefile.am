AUTOMAKE_OPTIONS= foreign  
CXXFLAGS += -g -Wall  -I ../../include -std=gnu++14
bin_PROGRAMS= EventServer
EventServer_SOURCES= acceptor.cpp socketholder.cpp Handler.cpp Message.cpp threadpools.cpp EventServer.cpp
EventServer_LDFLAGS = 
LIBS +=-levent -levent_pthreads -levent_core -lpthread
