#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct UnitVehicle
    {
        static constexpr const char* GetFileName() { return "UnitVehicle"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t vehicleTypeEnum;
        uint32_t numberPilots;
        uint32_t pilotPosture00;
        uint32_t pilotPosture01;
        uint32_t numberPassengers;
        uint32_t passengerPosture00;
        uint32_t passengerPosture01;
        uint32_t passengerPosture02;
        uint32_t passengerPosture03;
        uint32_t passengerPosture04;
        uint32_t passengerPosture05;
        uint32_t numberGunners;
        uint32_t gunnerPosture00;
        uint32_t gunnerPosture01;
        uint32_t gunnerPosture02;
        uint32_t gunnerPosture03;
        uint32_t gunnerPosture04;
        uint32_t gunnerPosture05;
        uint32_t vendorItemPrice;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdTooltip;
        std::wstring buttonIcon;
        uint32_t flags;
        uint32_t soundEventIdTakeoff;
        uint32_t soundEventIdLanding;
        uint32_t waterSurfaceEffectIdMoving;
        uint32_t waterSurfaceEffectIdStanding;
        uint32_t waterSurfaceEffectIdJumpIn;
        uint32_t waterSurfaceEffectIdJumpOut;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            vehicleTypeEnum = file.getUint(recordIndex, col++);
            numberPilots = file.getUint(recordIndex, col++);
            pilotPosture00 = file.getUint(recordIndex, col++);
            pilotPosture01 = file.getUint(recordIndex, col++);
            numberPassengers = file.getUint(recordIndex, col++);
            passengerPosture00 = file.getUint(recordIndex, col++);
            passengerPosture01 = file.getUint(recordIndex, col++);
            passengerPosture02 = file.getUint(recordIndex, col++);
            passengerPosture03 = file.getUint(recordIndex, col++);
            passengerPosture04 = file.getUint(recordIndex, col++);
            passengerPosture05 = file.getUint(recordIndex, col++);
            numberGunners = file.getUint(recordIndex, col++);
            gunnerPosture00 = file.getUint(recordIndex, col++);
            gunnerPosture01 = file.getUint(recordIndex, col++);
            gunnerPosture02 = file.getUint(recordIndex, col++);
            gunnerPosture03 = file.getUint(recordIndex, col++);
            gunnerPosture04 = file.getUint(recordIndex, col++);
            gunnerPosture05 = file.getUint(recordIndex, col++);
            vendorItemPrice = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
            localizedTextIdTooltip = file.getUint(recordIndex, col++);
            buttonIcon = file.getString(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            soundEventIdTakeoff = file.getUint(recordIndex, col++);
            soundEventIdLanding = file.getUint(recordIndex, col++);
            waterSurfaceEffectIdMoving = file.getUint(recordIndex, col++);
            waterSurfaceEffectIdStanding = file.getUint(recordIndex, col++);
            waterSurfaceEffectIdJumpIn = file.getUint(recordIndex, col++);
            waterSurfaceEffectIdJumpOut = file.getUint(recordIndex, col++);
        }
    };
}