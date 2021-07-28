CC = g++
RM = rm -f
QT = /usr/include/qt4
QTLIB = /usr/lib/x86_64-linux-gnu
MOC= /usr/bin/moc
RCC = /usr/bin/rcc

PROGRAM = ess_qinghe2_ctrl
RCCS = 

OBJECTS = .obj/data_access.o \
		.obj/device.o \
		.obj/agvc_ctrl_main.o \
		.obj/main.o	\
		.obj/scada_report_manager.o


MOCS = 

INCLUDES = -I../../../../include -I../../../../ui/include -I$(QT) -I$(QT)/QtCore -I$(QT)/QtGui -I../../../../com/graph/include -I../../../../scada/scada_normal
DEFINES	 = -D_LINUX -DQT_OPENGL_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED -DQT_DLL -DQT_THREAD_SUPPORT 
CFLAGS   = $(DEFINES) -fPIC -D_MAIN_ARG
MOCFLAGS = $(DEFINES) 
LDFLAGS  = -fPIC
LIBS     = -L$(QT)/lib -L$(QTLIB) -L/$(RES3000)/bin -lQtGui -lQtCore -lQtXml -lGL  -lstdc++ -ldbms_d_v2 -ldnet_dll_v2 -lprivilege_check_interface_dll -lrdb_common_func -lxde_text_coder -lconfig_file_dll
ifeq ($(DEBUG), TRUE)
	CFLAGS  += -g
	LDFLAGS += -g
endif

all: ${PROGRAM}

${PROGRAM}: $(MOCS) $(RCCS) $(OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $(LIBS) ${OBJECTS} 
	cp $(PROGRAM)  $(RES3000)/bin

clean:
	$(RM) $(PROGRAM) $(OBJECTS) $(MOCS) $(RCCS) 

.obj/data_access.o:
	$(CC) -o .obj/data_access.o -c $(CFLAGS) $(INCLUDES) ../../data_access.cpp 
.obj/device.o:
	$(CC) -o .obj/device.o -c $(CFLAGS) $(INCLUDES) ../../device.cpp 	
.obj/agvc_ctrl_main.o:
	$(CC) -o .obj/agvc_ctrl_main.o -c $(CFLAGS) $(INCLUDES) ../../agvc_ctrl_main.cpp
.obj/main.o:
	$(CC) -o .obj/main.o -c $(CFLAGS) $(INCLUDES) ../../main.cpp	
.obj/scada_report_manager.o:
	$(CC) -o .obj/scada_report_manager.o -c $(CFLAGS) $(INCLUDES) ../../../../scada/scada_normal/scada_report_manager.cpp



