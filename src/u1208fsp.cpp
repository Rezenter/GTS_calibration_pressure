#include <labbot/kernel/drivers/DeviceDriver.h>
#include <jsoncpp/json/json.h>
#include "u1208fsp.h"
#include <stdio.h>
#include <unistd.h>


FLUG_DYNAMIC_DRIVER(U1208FSP);

U1208FSP::U1208FSP(const std::string &deviceInstance, const std::string &deviceType)
        : DeviceDriver(
            deviceInstance, deviceType
        )
{

}

U1208FSP::~U1208FSP() {

}

bool U1208FSP::loadConfig(Json::Value &config) {
    if (config.isMember("maxLength")) {
        m_pointsMaxLength = config["maxLength"].asUInt();
    }else{
        m_pointsMaxLength = 1000000;
    }
    return true;
}

bool U1208FSP::initModule() {
    m_dev = NULL;
    int ret = libusb_init(NULL);
    if (ret < 0) {
        std::cout << "usb_device_find_USB_MCC: Failed to initialize libusb" << std::endl;
        return false;
    }
    if (m_dev = usb_device_find_USB_MCC(USB1208FS_PLUS_PID, NULL)) {
        std::cout << "Success, found a USB 1208FS-Plus!" << std::endl;
    } else {
        std::cout << "Failure, did not find a USB 1208FS-Plus!" << std::endl;
        return false;
    }
    memset(m_ranges, 0x0, sizeof(m_ranges));
    usbBuildGainTable_DE_USB1208FS_Plus(m_dev, m_table_DE_AIN);  
    m_count = 512;
    m_ch.clear();
    m_online = false;
    return true;
}

bool U1208FSP::destroyModule() {
    cleanup_USB1208FS_Plus(m_dev);
    return true;
}

bool U1208FSP::handleRequest(LabBot::Request &req, LabBot::Response &resp) {
    std::string reqtype = req.m_json["reqtype"].asString();
    if (reqtype == "getData") {
        return handleGetData(req, resp);
    }
    if (reqtype == "startAIn") {
        return handleStartAIn(req, resp);
    }
    if (reqtype == "stopAIn") {
        return handleStopAIn(req, resp);
    }
    if (reqtype == "fail") {
        return handleFailFunction(req, resp);
    }
    return false;
}

bool U1208FSP::rebootModule() {
    m_online = false;
    usbAInScanStop_USB1208FS_Plus(m_dev);
	usbAInScanClearFIFO_USB1208FS_Plus(m_dev);
    return destroyModule() && initModule();
}

LabBot::Module::State U1208FSP::getState() {
    std::cout << "Status = " << usbStatus_USB1208FS_Plus(m_dev) << std::endl;
    return ST_ONLINE;
}

bool U1208FSP::handleGetData(LabBot::Request &req, LabBot::Response &resp) {
    Json::Value ret;
    if (!req.m_json.isMember("length")) {
        throw std::runtime_error("length is not specified");
    }
    if (!req.m_json["length"].isUInt()) {
        throw std::runtime_error("length is not a positive integer value");
    }
    size_t length = req.m_json["length"].asUInt();
    length = std::min(length, m_points.size());
    int count = 0;
    for(int i = m_points.size() - length; i < m_points.size(); i++){
        for(int ch = 0; ch < NCHAN_DE; ch++){
            if(std::find(m_ch.begin(), m_ch.end(), ch) != m_ch.end()) {
                ret["data"][ch][count] = m_points.at(i).analogData[ch];
                ret["ch"][ch] = ch; 
            } 
        }
        ret["period"][count] = m_points.at(i).period;
        count++;
    }
    ret["status"] = "success";
    resp = ret;
    return true;
}

bool U1208FSP::handleStartAIn(LabBot::Request &req, LabBot::Response &resp) {
    Json::Value ret;
    if(m_online){
        handleStopAIn(req, resp);
    }
    if (!req.m_json.isMember("freq")) {
        throw std::runtime_error("freq is not specified");
    }
    if (!req.m_json["freq"].isDouble()) {
        throw std::runtime_error("freq is not a positive double value");
    }
    m_freq = req.m_json["freq"].asDouble();
    if(!req.m_json.isMember("ch")){
        throw std::runtime_error("ch is not specified");
    }
    if(!req.m_json.isMember("ranges")){
        throw std::runtime_error("ranges is not specified");
    }
    if (req.m_json["ch"].size() != req.m_json["ranges"].size()) {
        throw std::runtime_error("Sizes of channels and ranges don't match");
    }
    if (req.m_json["ch"].size() > NCHAN_DE) {
        throw std::runtime_error("There are only eight channels(terminals) for analog outputs");
    }
    m_ch.clear();
    for(int i = 0; i < NCHAN_DE; i++){
        m_ranges[i] = 0;
    }
    m_binChannels = 0x0;
    for(size_t i = 0; i < req.m_json["ch"].size(); i++){
        uint8_t ch = req.m_json["ch"][(int)i].asUInt();
        uint8_t range = req.m_json["ranges"][(int)i].asUInt();
        if(ch > NCHAN_DE - 1) {
            throw std::runtime_error("Ch must not exceed " + NCHAN_DE);
        }
        if(range > NGAINS_USB1208FS_PLUS - 1) {
            throw std::runtime_error("Range must not exceed " + NGAINS_USB1208FS_PLUS);
        }
        m_ranges[ch] = range;
        m_binChannels += (0x1<<ch);
        m_ch.push_back(ch);
    }
    m_freq *= m_ch.size();//wtf?
	usbAInScanStop_USB1208FS_Plus(m_dev);
	usbAInScanClearFIFO_USB1208FS_Plus(m_dev);
	memset(m_dataAIn, 0xbeef, sizeof(m_dataAIn));
	sleep(1);
    usbAInScanConfigR_USB1208FS_Plus(m_dev, m_ranges);
	usbAInScanStart_USB1208FS_Plus(m_dev, m_count, 0x0, m_freq, m_binChannels, m_options);
    m_start = std::chrono::system_clock::now();                       
	m_online = true;
    ret["status"] = "success";
    resp = ret;
    return true;
}

bool U1208FSP::handleStopAIn(LabBot::Request &req, LabBot::Response &resp) {
    Json::Value ret;
    m_online = false;
	usbAInScanStop_USB1208FS_Plus(m_dev);
	usbAInScanClearFIFO_USB1208FS_Plus(m_dev);
	m_points.clear();
    ret["status"] = "success";
    resp = ret;
    return true;
}

bool U1208FSP::handleFailFunction(LabBot::Request &req, LabBot::Response &resp) {
    throw std::runtime_error("handleFailFunction always fails");
}

void U1208FSP::cyclicFunc(){
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - m_start;
    if(m_online && (elapsed_seconds.count() > (m_count*m_ch.size()/m_freq))){
        int AIn = usbAInScanRead_USB1208FS_Plus(m_dev, m_count, m_ch.size(), m_dataAIn, m_options, 1000);
	    if(AIn > 0){
	        usbAInScanStart_USB1208FS_Plus(m_dev, m_count, 0x0, m_freq, m_binChannels, m_options);
            m_start = std::chrono::system_clock::now();
	        for (int i = 0; i < m_count; i++) {
	            U1208FSP::point point;
	            point.period = m_ch.size()/m_freq;
	            int j = 0; 
	            for(uint8_t& ch: m_ch){
	                point.analogData[ch] = volts_USB1208FS_Plus(rint(m_dataAIn[m_ch.size() * i + j]*m_table_DE_AIN[m_ranges[ch]][ch][0] + m_table_DE_AIN[m_ranges[ch]][ch][1]), m_ranges[ch]);
	                j++;
	            }
	            for(; j < NCHAN_DE; j++){
	                for(uint8_t ch = 0; ch < NCHAN_DE; ch++){
	                    if(std::find(m_ch.begin(), m_ch.end(), ch) == m_ch.end()) {
                            point.analogData[ch] = 0;
                        }
	                }
	            }
	            if(m_pointsMaxLength < m_points.size()){
	                m_points.erase(m_points.begin(), m_points.begin() + m_pointsMaxLength/2);
	            }
	            m_points.push_back(point);
	        }
	    }else{
	        /*
	        m_online = false;
	        usbAInScanStop_USB1208FS_Plus(m_dev);
	        usbAInScanClearFIFO_USB1208FS_Plus(m_dev);
	        std::cout << "Read error " << AIn << std::endl;
	        //throw std::runtime_error("Read error " + AIn);
	        */
	        std::cout << "Read error: " << AIn << std::endl;
	    }
    }
}


