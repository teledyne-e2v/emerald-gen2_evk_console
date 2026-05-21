#include <pigentl/sdk/sdk.h>
#define PFNC_INCLUDE_HELPERS
#include <genicam/PFNC.h>
#include <exception>
#include <iostream>
#include <string>
#include <limits>
#include <cstring>
#include <cstdlib>
#include <windows.h>

#ifdef WITH_CV
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif
#include <cassert>
#include <cstdio>
#include <sstream>


#define MAX_TIMEOUT_ACQ_IN_MS				3000  // 3000 ms			 // Note set to 0xFFFFFFFF for INFINITE timeout

#define _STR(x) #x
#define STR(x) _STR(x)
#define CHECK_API(call) { int status = call; if(status != CAM_ERR_SUCCESS) throw CMyException(#call,status); } (void)0




class CMyException : public std::exception
{
	std::string message;
	int error;
public:
	CMyException(const char * pcMsg, int nError); 
	virtual ~CMyException() throw() {}	
	const char* what();
};

CMyException::CMyException(const char * pcMsg, int nError) : message(pcMsg), error(nError)
{
	if(error == CAM_ERR_SUCCESS)
		return;

	std::stringstream ss;
	ss << message << " code: " << error;
	char pcErrText[512];
	size_t iSize =  sizeof(pcErrText);
	if (PiGentlSdkGetLastError(&error, pcErrText, &iSize) == CAM_ERR_SUCCESS)
		ss << " details: " << pcErrText;
	message = ss.str();
}

const char* CMyException::what()
{
	return message.c_str();
}

uint32_t run;
int k=0;

#ifdef WITH_CV
int PixelFormatPFNCToCV(PfncFormat pfnc){

    switch(pfnc){
        case BGR8: return CV_8UC3;
        default : break;
    }
    switch(PFNC_PIXEL_SIZE(pfnc))
    {
        case 8: return CV_8U;
        case 16: return CV_16U;
        default:
            std::cerr << "unsupported pixel format " << GetPixelFormatName(pfnc) << std::endl;
            throw std::logic_error("unsupported pixel format");

    }
}
#endif

void hexDump(const void *ptr, size_t buflen) {
  unsigned char *buf = (unsigned char*)ptr;
  unsigned i, j;
  for (i=0; i<buflen; i+=16) {
    printf("%06x: ", i);
    for (j=0; j<16; j++)
      if (i+j < buflen)
        printf("%02x ", buf[i+j]);
      else
        printf("   ");
    printf(" ");
    for (j=0; j<16; j++)
      if (i+j < buflen)
        printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
    printf("\n");
  }
}

struct config_s{
    uint32_t addr;
    uint32_t value;
    uint32_t size;
};

void load_config(struct config_s* config, CAM_HANDLE handle)
{
    while(config->size != 0){
        assert(config->size <= sizeof(config->value));
        size_t size=config->size;
        CHECK_API(PiGentlSdkWriteRegister(handle, config->addr, &config->value, &size));
        config++;
    }
}
#define UNUSED(x) (void)x 
int main(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);
#ifdef __unix__
    run = 1;
	//configure signal handler for Ctrl+C
	signal(SIGINT, term);
#else
    if(argc < 2 || atoi(argv[1]) < 1){
        std::cerr << "grabbing 1 image" << std::endl;
    }else{
        run = atoi(argv[1]);
    }
#endif

    run = 1;

    CAM_HANDLE hCamera = NULL;

	try
	{
		CHECK_API(PiGentlSdkInitializeLibrary());

		unsigned long ulNbCameras;

		CHECK_API(PiGentlSdkUpdateCameraList(&ulNbCameras));

		std::cout << ulNbCameras << " camera(s) found" << std::endl;

		if (ulNbCameras == 0){
			std::cerr << "No camera found" << std::endl;
			return 0;
		}

		tCameraInfo CameraInfo;
		for (unsigned long ulIndex = 0; ulIndex < ulNbCameras; ulIndex ++){
			CHECK_API(PiGentlSdkGetCameraInfo(ulIndex, &CameraInfo));
		}

        //open the first camera in the list
		CHECK_API(PiGentlSdkOpenCamera(&CameraInfo, &hCamera));

        std::cout << "using vendor:" << CameraInfo.vendor
                      << ", model: " << CameraInfo.model
                      << ", serial: " << CameraInfo.serial << std::endl;

        if(0==strcmp(CameraInfo.model,"Emerald-Gen2")){

            // BASE ADDRESS DEFINITION
            uint32_t FirmwareGeneralAddr = 0x10000;
            uint32_t SensorAddr = 0x30000;
            uint32_t FirmwareTriggerAddr = 0x11000;
            uint32_t FPGAAddr = 0x20000;
            uint32_t reg_line_length = 0x0006;
            uint32_t reg_config1_tint_ll = 0x0020;
            float time_base = 0.02; //us

            uint32_t address;  
            uint16_t value;
            uint32_t value_fw ;
            size_t size ;
            int error;

            //read sensor chipID
            address=SensorAddr + 0x7F;
            size = sizeof(value);
            error=PiGentlSdkReadRegister(hCamera,address,&value,&size);
            printf("READ:  Address=0x%x Value=0x%x / %d\n",address,value,value);
           
            //setup Mono8 pixel format
            value_fw = Mono8;
            address = FirmwareGeneralAddr + 0x0014;
            size = sizeof(value_fw);
            PiGentlSdkWriteRegister(hCamera,address,&value_fw,&size);
            printf("WRITE:  Address=0x%x Value=0x%x / %d\n",address,value_fw,value_fw);

            //control exposure time to 20ms
            uint16_t line_length=0;
            uint16_t tint=20000; //us
    
            address=SensorAddr + reg_line_length;
            size = sizeof(value);
            error=PiGentlSdkReadRegister(hCamera,address,&value,&size);
            printf("READ:  Address=0x%x Value=0x%x / %d\n",address,value,value);
            line_length = value;
            
            value = (uint16_t)((tint/line_length)/time_base);
            address=SensorAddr + reg_config1_tint_ll;
            size = sizeof(value);
            error=PiGentlSdkWriteRegister(hCamera,address,&value,&size);
            printf("WRITE:  Address=0x%x Value=0x%x / %d\n",address,value,value);

        }
        
        //using GenTL directly, this number can be retrieved from the device (MIN_BUFFERS)
		size_t iNbOfBuffer = 10;
		CHECK_API(PiGentlSdkSetNumberOfBuffers(hCamera, iNbOfBuffer));

		std::cout << "starting acquisition, ESC to stop" << std::endl;
		CHECK_API(PiGentlSdkStartAcquisition(hCamera));

		unsigned long ulNBImageAcquired = 0;
        int rescale_factor = 4;

		while (run){
			tImageInfos ImageInfos;
			int Error = PiGentlSdkGetBuffer(hCamera, &ImageInfos,  MAX_TIMEOUT_ACQ_IN_MS);
			if(Error == CAM_ERR_SUCCESS){

                if(!ImageInfos.isIncomplete && ImageInfos.isNewData){
                    ulNBImageAcquired ++;
                    //std::cout << "\r got " << ulNBImageAcquired << " images" << std::flush;

                    // ******************************************
                    // Do something with image ...
                    // ImageInfos.pDatas => contains raw data buffser
                    // ImageInfos.iImageWidth;
                    // ImageInfos.iImageHeight;
                    // ImageInfos.eImagePixelType;
                    // ImageInfos.iImageSize
                    // ImageInfos.iLinePitch
                    // ImageInfos.iBlockId;
                    // ImageInfos.iTimestamp;
                    // ImageInfos.iNbPacketLost;
                    // ImageInfos.iNbFrameLost;
                    // ImageInfos.iNbImageAcquired;
                    // ******************************************
                    //hexDump(ImageInfos.pDatas,16);
                    //std::cout << "new image " <<  ImageInfos.iImageWidth << "x" << ImageInfos.iImageHeight << std::endl;
                    #ifdef WITH_CV

                    cv::Mat mat(ImageInfos.iImageHeight,ImageInfos.iImageWidth,PixelFormatPFNCToCV((PfncFormat)ImageInfos.iPFNC32),ImageInfos.pDatas);
                    cv::Mat display;
                    cv::resize(mat, display, cv::Size(mat.size().width / rescale_factor, mat.size().height / rescale_factor));
                    cv::imshow("TDYGentlSdkMonothreadExample",display);

                    #else
                    hexDump(ImageInfos.pDatas,16);
                    #endif
                }
                #ifdef WITH_CV
                k = cv::waitKey(1);
                if(k == 'q' || k == 27)
                {
                    std::cout << "Live preview stop" << std::endl;
                    run = 0;
                }
                #endif
				//You must requeue the buffer to avoid buffer starvation
				CHECK_API(PiGentlSdkRequeueBuffer(hCamera, ImageInfos.hBuffer));
            }else{
                char buffer[256];
                size_t buffer_size = sizeof(buffer);
                CHECK_API(PiGentlSdkGetLastError(&Error,buffer,&buffer_size));
                if(buffer_size)
                    std::cerr << " ERROR: " << buffer << std::endl;
			}
        }

	}catch (const std::exception &e){
		std::cerr << " FATAL ERROR: " << e.what() << std::endl;
	}

	if(hCamera){
		CHECK_API(PiGentlSdkStopAcquisition(hCamera));
		CHECK_API(PiGentlSdkFlushBuffers(hCamera));
		CHECK_API(PiGentlSdkCloseCamera(hCamera));
	}
	std::cout << "closing library..." << std::endl;

	CHECK_API(PiGentlSdkTerminateLibrary());

	return 0;
}
