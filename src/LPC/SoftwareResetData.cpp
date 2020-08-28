
//SD modified by GA for latest RRF

#include "SoftwareResetData.h"
#include "RepRapFirmware.h"


/*
 
 On LPC17xx we will save SoftwareReset Data to the final sector. The last sector is 32K in size.
 Data must be written in 256 or 512 or 1024 or 4096 bytes.

 The LPC1768/9 doesn't have the page erase IAP command, so we have to use the whole sector
 with a write of 512bytes = 64 slots...

 */


/* Last sector address */
#define START_ADDR_LAST_SECTOR  0x00078000

constexpr uint32_t IAP_PAGE_SIZE  = 256;
constexpr uint32_t SLOT_SIZE = 2*IAP_PAGE_SIZE;
constexpr uint32_t MAX_SLOT = ((32*1024)/SLOT_SIZE) - 1;
/* LAST SECTOR */
#define IAP_LAST_SECTOR         29
static uint32_t currentSlot     = MAX_SLOT+1;

//When the Sector is erased, all the bits will be high
//This checks if the first 4 bytes are all high for the designated software reset slot
//the first 2 bytes of a used reset slot will have the magic number in it.
bool LPC_IsSoftwareResetDataSlotVacant(uint8_t slot)
{
    const uint32_t *p = (uint32_t *) (START_ADDR_LAST_SECTOR + (slot*SLOT_SIZE));
    
    for (size_t i = 0; i < SLOT_SIZE/sizeof(uint32_t); ++i)
    {
        if (*p != 0xFFFFFFFF)
        {
            return false;
        }
        ++p;
    }
    return true;
 }

void LPC_ReadSoftwareResetData(void *data, uint32_t dataLength)
{
    // find the most recently written data or slot 0 if all free
    uint32_t currentSlot = MAX_SLOT;
    while (currentSlot > 0 && LPC_IsSoftwareResetDataSlotVacant(currentSlot))
        currentSlot--;
    uint32_t *slotStartAddress = (uint32_t *) (START_ADDR_LAST_SECTOR + (currentSlot*SLOT_SIZE));
    memcpy(data, slotStartAddress, dataLength);
}


bool LPC_EraseSoftwareResetData()
{
    // interrupts will be disabled before these are called.
    //__disable_irq(); //disable Interrupts

    uint8_t iap_ret_code;
    bool ret = false;

    // Have we reached the last slot yet?
    if (currentSlot < MAX_SLOT)
    {
        currentSlot++;
        return true;
    }
    /* Prepare to write/erase the last sector */
    iap_ret_code = Chip_IAP_PreSectorForReadWrite(IAP_LAST_SECTOR, IAP_LAST_SECTOR);
    
    /* Error checking */
    if (iap_ret_code != IAP_CMD_SUCCESS) {
        debugPrintf("Chip_IAP_PreSectorForReadWrite() failed to execute, return code is: %x\r\n", iap_ret_code);
        ret = false;
    }
    else
    {

        /* Erase the last Sector */
        iap_ret_code = Chip_IAP_EraseSector(IAP_LAST_SECTOR,IAP_LAST_SECTOR);
        
        /* Error checking */
        if (iap_ret_code != IAP_CMD_SUCCESS) {
            debugPrintf("Chip_IAP_ErasePage() failed to execute, return code is: %x\r\n", iap_ret_code);
            ret =  false;
        }
        else
        {
            ret = true;
        }
    
    
    }
    //__enable_irq();
    currentSlot = 0;    
    return ret;
    
}

bool LPC_WriteSoftwareResetData(const void *data, uint32_t dataLength){
    uint8_t iap_ret_code;
    bool ret = false;
    if (dataLength != SLOT_SIZE)
    {
        debugPrintf("Bad flash data size\n");
        return false;
    }
    if (currentSlot > MAX_SLOT)
    {
        debugPrintf("Write to flash slot that has not been read\n");
        return false;
    }
    //
    
    uint32_t slotStartAddress = (START_ADDR_LAST_SECTOR + (currentSlot*SLOT_SIZE));
    
    //__disable_irq(); //disable Interrupts
    
    /* Prepare to write/erase the last sector */
    iap_ret_code = Chip_IAP_PreSectorForReadWrite(IAP_LAST_SECTOR, IAP_LAST_SECTOR);
    
    /* Error checking */
    if (iap_ret_code != IAP_CMD_SUCCESS) {
        debugPrintf("Chip_IAP_PreSectorForReadWrite() failed to execute, return code is: %x\r\n", iap_ret_code);
        ret = false;
    }
    else
    {
        //debugPrintf("About to write %d bytes to address %x", IAP_PAGE_SIZE, slotStartAddress);
        /* Write to the last sector */
        iap_ret_code = Chip_IAP_CopyRamToFlash(slotStartAddress, (uint32_t *)data, SLOT_SIZE);

        /* Error checking */
        if (iap_ret_code != IAP_CMD_SUCCESS) {
            debugPrintf("Chip_IAP_CopyRamToFlash() failed to execute, return code is: %x\r\n", iap_ret_code);
            ret =  false;
        }
        else
        {
            ret = true;
        }
    }
    /* Re-enable interrupt mode */
    //__enable_irq();
    
    return ret;
    
}

