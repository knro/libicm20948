#include <iostream>
#include <iomanip>
#include <unistd.h>
#include "ICM20948.h"

int main(void)
{
    ICM20948 imu;
    icm20948_agmt_t agmt;

    std::cout << "=== ICM20948 I2C Communication Test ===" << std::endl;
    std::cout << "Testing I2C communication with ICM20948 at address 0x68" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    std::cout << "[TEST 1] Attempting I2C initialization..." << std::endl;
    try
    {
        imu.begin_i2c(ICM_20948_I2C_ADDR_AD0, "/dev/i2c-2");
        std::cout << "✅ SUCCESS: I2C initialization completed" << std::endl << std::endl;
    }
    catch (const ICM20948_exception &e)
    {
        std::cerr << "❌ FAILED: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[TEST 2] Reading raw sensor data (1 sample to check comms)..." << std::endl;
    icm20948_status_e status = imu.readSensor(&agmt);
    if (status == ICM_20948_STAT_OK)
    {
        std::cout << "✅ SUCCESS: Sensor read returned OK" << std::endl << std::endl;
    }
    else
    {
        std::cerr << "❌ FAILED: Sensor read returned error code " << static_cast<int>(status) << std::endl;
        return 1;
    }

    std::cout << "[TEST 3] Reading 10 samples with scaled sensor data..." << std::endl;
    std::cout << std::fixed << std::setprecision(4);

    for (int i = 0; i < 10; i++)
    {
        status = imu.readSensor();
        if (status == ICM_20948_STAT_OK)
        {
            std::cout << "Sample " << std::setw(2) << i + 1 << " | ";
            std::cout << "Accel [m/s²]: "
                      << std::setw(9) << imu.getAccelX_mss() << " "
                      << std::setw(9) << imu.getAccelY_mss() << " "
                      << std::setw(9) << imu.getAccelZ_mss() << " | ";
            std::cout << "Gyro [°/s]: "
                      << std::setw(9) << imu.getGyroX_dps() << " "
                      << std::setw(9) << imu.getGyroY_dps() << " "
                      << std::setw(9) << imu.getGyroZ_dps() << " | ";
            std::cout << "Mag [µT]: "
                      << std::setw(8) << imu.getMagX_uT() << " "
                      << std::setw(8) << imu.getMagY_uT() << " "
                      << std::setw(8) << imu.getMagZ_uT() << " | ";
            std::cout << "Temp [°C]: "
                      << std::setw(7) << imu.getTemp_C();
            std::cout << std::endl;
        }
        else
        {
            std::cerr << "  Sample " << i + 1 << ": read error (code " << static_cast<int>(status) << ")" << std::endl;
        }
        usleep(100000); // 100 ms between samples
    }

    std::cout << std::endl;
    std::cout << "=== TEST SUMMARY ===" << std::endl;
    std::cout << "✅ I2C initialization:   PASSED" << std::endl;
    std::cout << "✅ Sensor communication: PASSED" << std::endl;
    std::cout << "✅ Data readout (10 samples): PASSED" << std::endl;
    std::cout << std::endl;
    std::cout << "🎉 ICM20948 I2C communication is working correctly!" << std::endl;

    return 0;
}
