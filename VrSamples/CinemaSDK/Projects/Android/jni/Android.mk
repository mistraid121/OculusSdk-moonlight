LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)					# clean everything up to prepare for a module

include ../../../../cflags.mk

LOCAL_MODULE    := cinema				# generate libcinema.so


LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../SampleFramework/Src \
					$(LOCAL_PATH)/../../../../../1stParty/OVR/Include


LOCAL_SRC_FILES	:= 	../../../Src/CinemaApp.cpp \
					../../../Src/Native.cpp \
					../../../Src/View.cpp \
					../../../Src/SceneManager.cpp \
					../../../Src/ViewManager.cpp \
					../../../Src/ShaderManager.cpp \
					../../../Src/ModelManager.cpp \
					../../../Src/AppManager.cpp \
					../../../Src/PcManager.cpp \
					../../../Src/MoviePlayerView.cpp \
                    ../../../Src/SelectionView.cpp \
					../../../Src/PcSelectionView.cpp \
					../../../Src/AppSelectionView.cpp \
					../../../Src/TheaterSelectionView.cpp \
					../../../Src/TheaterSelectionComponent.cpp \
					../../../Src/CarouselBrowserComponent.cpp \
					../../../Src/PcCategoryComponent.cpp \
					../../../Src/MoviePosterComponent.cpp \
					../../../Src/MovieSelectionComponent.cpp \
					../../../Src/CarouselSwipeHintComponent.cpp \
					../../../Src/CinemaStrings.cpp \
					../../../Src/Settings.cpp

LOCAL_LDLIBS 			:= -llog -landroid -lGLESv3 -lEGL -lz
LOCAL_STATIC_LIBRARIES += sampleframework # a voir si necessaire android_native_app_glue
LOCAL_SHARED_LIBRARIES += vrapi

include $(BUILD_SHARED_LIBRARY)			# start building based on everything since CLEAR_VARS

$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
$(call import-module,VrSamples/SampleFramework/Projects/Android/jni)
