bin_PROGRAMS = smclient smserver iprulesetter

iprulesetter_SOURCES = iptables/iprulesetter.cpp

smclient_SOURCES = BasicClient.h	BasicClient.cpp		\
   UProxy.h			UProxy.cpp		\
   ProxyShuffler.h		ProxyShuffler.cpp 	\
   UDPRelay.h			UDPRelay.cpp 		\
   TunnelStream.h		Packet.h		\
   SocksStream.h		SocksStream.cpp		\
   Packetizer.h			Packetizer.cpp		\
   Util.h			Util.cpp		\
   SendingBuffer.h		SendingBuffer.cpp	\
   RecvingBuffer.h		RecvingBuffer.cpp	\
   Crypto.h			Crypto.cpp		\
   curve25519-donna.h		curve25519-donna.c	\
   debug.h			debug.cpp		\
   dream/dream.h		dream/dream.c		\
   dream/mmio.h			dream/mmio.c		\
   Morpher.h			Morpher.cpp		\
   KeyMsg.h			KeyMsg.cpp		\
   Skype.h			Skype.cpp		\
   SkypeClient.h		SkypeClient.cpp		\
   SMClientMain.cpp		version.h		\
   version.c			Config.h			\
   Config.cpp			SkypePublicAPIWrapper.h	\
   SkypePublicAPI.h	   SkypePublicAPI.cpp    \
   SkypePublicAPIWrapperClient.cpp   SkypePublicAPIWrapperClient.h 

smclient_CPPFLAGS = $(AM_CPPFLAGS) $(CYASSL_CPPFLAGS) $(SKYPE_CPPFLAGS) $(BOOST_CPPFLAGS) $(PAPI_CFLAGS) $(EXTRA_CPPFLAGS) -DCYASSL_KEY_GEN -DSSL_LIB_OPENSSL
smclient_CFLAGS = $(AM_CFLAGS) $(EXTRA_CFLAGS) 
smclient_CXXFLAGS = $(smclient_CFLAGS) 
smclient_LDFLAGS = $(CYASSL_LDFLAGS) $(SKYPE_LDFLAGS) $(BOOST_LDFLAGS)  -pthread 
smclient_LDADD_TMP = $(CYASSL_LIBS) $(SKYPE_LIBS) $(BOOST_LIBS) $(BOOST_SYSTEM_LIB) $(BOOST_ASIO_LIB) $(BOOST_THREAD_LIB) $(PAPI_LIBS) $(WIN32_LIBS)

if SKYPE_PAPI
smclient_LDADD = $(smclient_LDADD_TMP)
else 
smclient_LDADD = $(smclient_LDADD_TMP) -lskypekit-cppwrapper_2_lib -lskypekit-cyassl_lib
endif

if TPROXY_ENABLED
install-exec-hook:
	 sudo chmod u+s $(bindir)/iprulesetter
	sudo chmod 777 $(bindir)
endif
smserver_SOURCES = BasicClient.h	BasicClient.cpp		\
   ClientHandle.h	ClientHandle.cpp		\
   Server.h			Server.cpp		\
   ProxyShuffler.h		ProxyShuffler.cpp 	\
   Relay.h			Relay.cpp		\
   RelayStream.h		RelayStream.cpp		\
   Packetizer.h			Packetizer.cpp		\
   TunnelStream.h		Packet.h		\
   Util.h			Util.cpp		\
   SendingBuffer.h		SendingBuffer.cpp	\
   RecvingBuffer.h		RecvingBuffer.cpp	\
   Crypto.h			Crypto.cpp		\
   curve25519-donna.h		curve25519-donna.c	\
   debug.h			debug.cpp		\
   dream/dream.h		dream/dream.c		\
   dream/mmio.h			dream/mmio.c		\
   Morpher.h			Morpher.cpp		\
   KeyMsg.h			KeyMsg.cpp		\
   Skype.h			Skype.cpp		\
   SkypeServer.h		SkypeServer.cpp		\
   SMServerMain.cpp		version.h		\
   version.c			Config.h		\
   Config.cpp			SkypePublicAPIWrapper.h	\
   SkypePublicAPI.h		  SkypePublicAPI.cpp	\
   SkypePublicAPIWrapperServer.cpp    SkypePublicAPIWrapperServer.h

smserver_CPPFLAGS = $(AM_CPPFLAGS) $(CYASSL_CPPFLAGS) $(SKYPE_CPPFLAGS) $(BOOST_CPPFLAGS) $(PAPI_CFLAGS) $(EXTRA_CPPFLAGS) -DCYASSL_KEY_GEN -DSSL_LIB_OPENSSL

smserver_CFLAGS = $(AM_CFLAGS) $(EXTRA_CFLAGS)
smserver_CXXFLAGS = $(smserver_CFLAGS) 
smserver_LDFLAGS = $(CYASSL_LDFLAGS) $(SKYPE_LDFLAGS) $(BOOST_LDFLAGS) -pthread 
smserver_LDADD_TMP = $(CYASSL_LIBS) $(SKYPE_LIBS) $(BOOST_LIBS) $(BOOST_SYSTEM_LIB) $(BOOST_ASIO_LIB) $(BOOST_THREAD_LIB) $(PAPI_LIBS) $(WIN32_LIBS)

if SKYPE_PAPI
smserver_LDADD = $(smserver_LDADD_TMP)
else
smserver_LDADD = $(smserver_LDADD_TMP) -lskypekit-cppwrapper_2_lib -lskypekit-cyassl_lib
endif


genversion: 
	echo 'const char * skypemorph_revision = "'`$(srcdir)/version.sh $(srcdir)`'";' > version.c

version.c: genversion

check_PROGRAMS = sending-test recving-test morpher-test buffer-test

sending_test_SOURCES = SendingBuffer.h SendingBuffer.cpp Packet.h Util.h \
   Util.cpp debug.h debug.cpp 
sending_test_CPPFLAGS = $(AM_CPPFLAGS) -DDEBUG -UNDEBUG -DUNITTEST
sending_test_CXXFLAGS = $(AM_CFLAGS) $(EXTRA_CFLAGS) -g -O0 -Wall 

recving_test_SOURCES = RecvingBuffer.h RecvingBuffer.cpp Packet.h Util.h \
   Util.cpp debug.h debug.cpp 
recving_test_CPPFLAGS = $(AM_CPPFLAGS) -DDEBUG -UNDEBUG -DUNITTEST
recving_test_CXXFLAGS = $(AM_CFLAGS) $(EXTRA_CFLAGS) -g -O0 -Wall 

morpher_test_SOURCES = Morpher.h Morpher.cpp Packet.h Util.h 			\
   Util.cpp debug.h debug.cpp    dream/dream.h		dream/dream.c		\
   dream/mmio.h			dream/mmio.c
morpher_test_CPPFLAGS = $(AM_CPPFLAGS) -DDEBUG -UNDEBUG -DUNITTEST 
morpher_test_CXXFLAGS = $(AM_CFLAGS) $(EXTRA_CFLAGS) -g -O0 -Wall 

buffer_test_SOURCES = SendingBuffer.h SendingBuffer.cpp RecvingBuffer.h RecvingBuffer.cpp \
	buffertest.cc Packet.h Util.h Util.cpp debug.h debug.cpp 
buffer_test_CPPFLAGS = $(AM_CPPFLAGS) -DDEBUG -UNDEBUG
buffer_test_CXXFLAGS = $(AM_CFLAGS) $(EXTRA_CFLAGS) -g -O0 -Wall 

TESTS = sending-test recving-test morpher-test buffer-test
