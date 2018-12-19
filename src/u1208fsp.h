#ifndef U1208FSP_H
#define U1208FSP_H

#include "pmd.h"
#include "usb-1208FS-Plus.h"
#include <chrono>

class U1208FSP : public LabBot::DeviceDriver {
public:
    U1208FSP() = delete;
    ~U1208FSP() override ;
    U1208FSP (const std::string & deviceInstance, const std::string & deviceType);

    bool loadConfig (Json::Value & config) override;
    bool initModule () override;
    bool destroyModule () override;
    bool handleRequest (LabBot::Request & req, LabBot::Response & resp) override;
    bool rebootModule () override;
    LabBot::Module::State getState() override;
    void cyclicFunc() override;

    struct point{
        double analogData[NCHAN_DE];
        double period;
    };

protected:
    bool handleGetData (LabBot::Request & req, LabBot::Response & resp);
    bool handleFailFunction (LabBot::Request & req, LabBot::Response & resp);
    bool handleStartAIn(LabBot::Request & req, LabBot::Response & resp);
    bool handleStopAIn(LabBot::Request & req, LabBot::Response & resp);
    
    
private:
    libusb_device_handle* m_dev = NULL;
    float m_table_DE_AIN[NGAINS_USB1208FS_PLUS][NCHAN_DE][2];
    uint8_t m_options = DIFFERENTIAL_MODE;
    uint8_t m_ranges[NCHAN_SE];//had to use NCHAN_SE even if used in Diff mode
    uint32_t m_count;
    uint16_t m_dataAIn[NCHAN_DE*512];  // holds not more than 512 points per channel
    bool m_online;
    double m_freq;
    uint8_t m_binChannels;
    std::vector<uint8_t> m_ch;
    size_t m_pointsMaxLength;
    std::vector<U1208FSP::point> m_points;
    std::chrono::time_point<std::chrono::system_clock> m_start;
};


#endif //U1208FSP_H
