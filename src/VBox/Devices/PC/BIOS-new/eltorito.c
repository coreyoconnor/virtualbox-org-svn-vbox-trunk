/*
 * Copyright (C) 2006-2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 *  ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
 *
 *  Copyright (C) 2002  MandrakeSoft S.A.
 *
 *    MandrakeSoft S.A.
 *    43, rue d'Aboukir
 *    75002 Paris - France
 *    http://www.linux-mandrake.com/
 *    http://www.mandrakesoft.com/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */


#include <stdint.h>
#include <string.h>
#include "inlines.h"
#include "biosint.h"
#include "ebda.h"
#include "ata.h"

#if DEBUG_ELTORITO
#  define BX_DEBUG_INT13_ET(...)    BX_DEBUG(__VA_ARGS__)
#else
#  define BX_DEBUG_INT13_ET(...)
#endif

#if DEBUG_INT13_CD
#  define BX_DEBUG_INT13_CD(...)    BX_DEBUG(__VA_ARGS__)
#else
#  define BX_DEBUG_INT13_CD(...)
#endif

//@todo: call this something else? ET_BOOT?
#if DEBUG_ELTORITO
#  define BX_DEBUG_ELTORITO(...)    BX_DEBUG(__VA_ARGS__)
#else
#  define BX_DEBUG_ELTORITO(...)
#endif


//@todo: put in a header
#define AX      r.gr.u.r16.ax
#define BX      r.gr.u.r16.bx
#define CX      r.gr.u.r16.cx
#define DX      r.gr.u.r16.dx
#define SI      r.gr.u.r16.si
#define DI      r.gr.u.r16.di
#define BP      r.gr.u.r16.bp
#define ELDX    r.gr.u.r16.sp
#define DS      r.ds
#define ES      r.es
#define FLAGS   r.ra.flags.u.r16.flags


// ---------------------------------------------------------------------------
// Start of El-Torito boot functions
// ---------------------------------------------------------------------------

// !! TODO !! convert EBDA accesses to far pointers 

extern  int     diskette_param_table;


void BIOSCALL cdemu_init(void)
{
    // @TODO: a macro or a function for getting the EBDA segment
    uint16_t    ebda_seg = read_word(0x0040,0x000E);
    
    // the only important data is this one for now
    write_byte(ebda_seg,(uint16_t)&EbdaData->cdemu.active, 0x00);
}

uint8_t BIOSCALL cdemu_isactive(void)
{
    // @TODO: a macro or a function for getting the EBDA segment
    uint16_t    ebda_seg = read_word(0x0040,0x000E);
    
    return read_byte(ebda_seg,(uint16_t)&EbdaData->cdemu.active);
}

uint8_t BIOSCALL cdemu_emulated_drive(void)
{
    // @TODO: a macro or a function for getting the EBDA segment
    uint16_t    ebda_seg = read_word(0x0040,0x000E);
    
    return read_byte(ebda_seg,(uint16_t)&EbdaData->cdemu.emulated_drive);
}

// ---------------------------------------------------------------------------
// Start of int13 for eltorito functions
// ---------------------------------------------------------------------------

void BIOSCALL int13_eltorito(disk_regs_t r)
{
    // @TODO: a macro or a function for getting the EBDA segment
    uint16_t        ebda_seg=read_word(0x0040,0x000E);
    cdemu_t __far   *cdemu;

    cdemu = ebda_seg :> &EbdaData->cdemu;

    
    BX_DEBUG_INT13_ET("%s: AX=%04x BX=%04x CX=%04x DX=%04x ES=%04x\n", __func__, AX, BX, CX, DX, ES);
    // BX_DEBUG_INT13_ET("%s: SS=%04x DS=%04x ES=%04x DI=%04x SI=%04x\n", __func__, get_SS(), DS, ES, DI, SI);
    
    switch (GET_AH()) {

    // FIXME ElTorito Various. Should be implemented
    case 0x4a: // ElTorito - Initiate disk emu
    case 0x4c: // ElTorito - Initiate disk emu and boot
    case 0x4d: // ElTorito - Return Boot catalog
        BX_PANIC("%s: call with AX=%04x. Please report\n", __func__, AX);
        goto int13_fail;
        break;

    case 0x4b: // ElTorito - Terminate disk emu
        // FIXME ElTorito Hardcoded
        //@todo: maybe our cdemu struct should match El Torito to allow memcpy()?
        write_byte(DS,SI+0x00,0x13);
        write_byte(DS,SI+0x01,cdemu->media);
        write_byte(DS,SI+0x02,cdemu->emulated_drive);
        write_byte(DS,SI+0x03,cdemu->controller_index);
        write_dword(DS,SI+0x04,cdemu->ilba);
        write_word(DS,SI+0x08,cdemu->device_spec);
        write_word(DS,SI+0x0a,cdemu->buffer_segment);
        write_word(DS,SI+0x0c,cdemu->load_segment);
        write_word(DS,SI+0x0e,cdemu->sector_count);
        write_byte(DS,SI+0x10,cdemu->vdevice.cylinders);
        write_byte(DS,SI+0x11,cdemu->vdevice.spt);
        write_byte(DS,SI+0x12,cdemu->vdevice.heads);
        
        // If we have to terminate emulation
        if(GET_AL() == 0x00) {
            // FIXME ElTorito Various. Should be handled accordingly to spec
            cdemu->active = 0;  // bye bye
        }

        goto int13_success;
        break;

    default:
          BX_INFO("%s: unsupported AH=%02x\n", __func__, GET_AH());
          goto int13_fail;
          break;
    }

int13_fail:
    SET_AH(0x01); // defaults to invalid function in AH or invalid parameter
    SET_DISK_RET_STATUS(GET_AH());
    SET_CF();     // error occurred
    return;

int13_success:
    SET_AH(0x00); // no error
    SET_DISK_RET_STATUS(0x00);
    CLEAR_CF();   // no error
    return;
}

// ---------------------------------------------------------------------------
// End of int13 for eltorito functions
// ---------------------------------------------------------------------------

static const char isotag[]="CD001";
static const char eltorito[]="EL TORITO SPECIFICATION";
//
// Returns ah: emulated drive, al: error code
//
uint16_t cdrom_boot(void)
{
    // @TODO: a macro or a function for getting the EBDA segment
    uint16_t        ebda_seg=read_word(0x0040,0x000E);
    uint8_t         atacmd[12], buffer[2048];
    uint32_t        lba;
    uint16_t        boot_segment, nbsectors, i, error;
    uint8_t         device;
    uint8_t         read_try;
    cdemu_t __far   *cdemu;

    cdemu = ebda_seg :> &EbdaData->cdemu;

    // Find out the first cdrom
    for (device=0; device<BX_MAX_ATA_DEVICES;device++) {
        if (atapi_is_cdrom(device))
            break;
    }

    // if not found
    if(device >= BX_MAX_ATA_DEVICES) return 2;
    
    // Read the Boot Record Volume Descriptor
    _fmemset(&atacmd,0,12);
    atacmd[0]=0x28;                      // READ command
    atacmd[7]=(0x01 & 0xff00) >> 8;      // Sectors
    atacmd[8]=(0x01 & 0x00ff);           // Sectors
    atacmd[2]=(0x11 & 0xff000000) >> 24; // LBA
    atacmd[3]=(0x11 & 0x00ff0000) >> 16;
    atacmd[4]=(0x11 & 0x0000ff00) >> 8;
    atacmd[5]=(0x11 & 0x000000ff);

    for (read_try = 0; read_try <= 4; read_try++)
    {
        error = ata_cmd_packet(device, 12, &atacmd, 0, 2048L, ATA_DATA_IN, &buffer);
        if (!error)
            break;
    }
    if (error)
        return 3;

    // Validity checks
    if(buffer[0]!=0)
        return 4;
    for(i=0;i<5;i++){
        if(buffer[1+i] != isotag[i])
            return 5;
    }
    for(i=0;i<23;i++)
        if(buffer[7+i] != eltorito[i])
            return 6;

    //@todo: this swaps the LBA back and forth for no good reason??!
    // ok, now we calculate the Boot catalog address
    lba=buffer[0x4A]*0x1000000UL+buffer[0x49]*0x10000UL+buffer[0x48]*0x100UL+buffer[0x47];
    BX_DEBUG_ELTORITO("BRVD at LBA %lx\n", lba);

    // And we read the Boot Catalog
    _fmemset(&atacmd,0,12);
    atacmd[0]=0x28;                      // READ command
    atacmd[7]=(0x01 & 0xff00) >> 8;      // Sectors
    atacmd[8]=(0x01 & 0x00ff);           // Sectors
    atacmd[2]=(lba & 0xff000000) >> 24;  // LBA
    atacmd[3]=(lba & 0x00ff0000) >> 16;
    atacmd[4]=(lba & 0x0000ff00) >> 8;
    atacmd[5]=(lba & 0x000000ff);
    if((error = ata_cmd_packet(device, 12, &atacmd, 0, 2048L, ATA_DATA_IN, &buffer)) != 0)
        return 7;
    
    // Validation entry
    if(buffer[0x00]!=0x01)
        return 8;   // Header
    if(buffer[0x01]!=0x00)
        return 9;   // Platform
    if(buffer[0x1E]!=0x55)
        return 10;  // key 1
    if(buffer[0x1F]!=0xAA)
        return 10;  // key 2
    
    // Initial/Default Entry
    if(buffer[0x20]!=0x88)
        return 11; // Bootable

    BX_DEBUG_ELTORITO("Emulate drive %x\n", buffer[0x21]);
    cdemu->media = buffer[0x21];
    if(buffer[0x21]==0){
        // FIXME ElTorito Hardcoded. cdrom is hardcoded as device 0xE0.
        // Win2000 cd boot needs to know it booted from cd
        cdemu->emulated_drive = 0xE0;
    }
    else if(buffer[0x21]<4)
        cdemu->emulated_drive = 0x00;
    else
        cdemu->emulated_drive = 0x80;
    
    cdemu->controller_index = device/2;
    cdemu->device_spec      = device%2;
    
    boot_segment=buffer[0x23]*0x100+buffer[0x22];
    if(boot_segment==0x0000)boot_segment=0x07C0;
    
    cdemu->load_segment   = boot_segment;
    cdemu->buffer_segment = 0x0000;
    
    nbsectors=buffer[0x27]*0x100+buffer[0x26];
    cdemu->sector_count = nbsectors;

    //@todo: pointless swapping of the LBA value?
    lba=buffer[0x2B]*0x1000000UL+buffer[0x2A]*0x10000UL+buffer[0x29]*0x100UL+buffer[0x28];
    cdemu->ilba = lba;
    
    // And we read the image in memory
    _fmemset(&atacmd,0,12);
    atacmd[0]=0x28;                      // READ command
    atacmd[7]=((1+(nbsectors-1)/4) & 0xff00) >> 8;      // Sectors
    atacmd[8]=((1+(nbsectors-1)/4) & 0x00ff);           // Sectors
    atacmd[2]=(lba & 0xff000000) >> 24;  // LBA
    atacmd[3]=(lba & 0x00ff0000) >> 16;
    atacmd[4]=(lba & 0x0000ff00) >> 8;
    atacmd[5]=(lba & 0x000000ff);
    if((error = ata_cmd_packet(device, 12, &atacmd, 0, nbsectors*512L, ATA_DATA_IN, MK_FP(boot_segment,0))) != 0)
        return 12;
    
    // Remember the media type
    switch(cdemu->media) {
    case 0x01:  // 1.2M floppy
        cdemu->vdevice.spt       = 15;
        cdemu->vdevice.cylinders = 80;
        cdemu->vdevice.heads     = 2;
        break;
    case 0x02:  // 1.44M floppy
        cdemu->vdevice.spt       = 18;
        cdemu->vdevice.cylinders = 80;
        cdemu->vdevice.heads     = 2;
        break;
    case 0x03:  // 2.88M floppy
        cdemu->vdevice.spt       = 36;
        cdemu->vdevice.cylinders = 80;
        cdemu->vdevice.heads     = 2;
        break;
    case 0x04:  // Harddrive
        cdemu->vdevice.spt       = read_byte(boot_segment,446+6)&0x3f;
        cdemu->vdevice.cylinders = (read_byte(boot_segment,446+6)<<2) + read_byte(boot_segment,446+7) + 1;
        cdemu->vdevice.heads     = read_byte(boot_segment,446+5) + 1;
        break;
    }
    
    if(cdemu->media != 0) {
        // Increase bios installed hardware number of devices
        if(cdemu->emulated_drive == 0x00)
            write_byte(0x40,0x10,read_byte(0x40,0x10)|0x41);
        else
            write_byte(ebda_seg,(uint16_t)&EbdaData->ata.hdcount, read_byte(ebda_seg, (uint16_t)&EbdaData->ata.hdcount) + 1);
    }
    
    
    // everything is ok, so from now on, the emulation is active
    if(cdemu->media !=0 )
        cdemu->active = 0x01;
    
    // return the boot drive + no error
    return (cdemu->emulated_drive*0x100)+0;
}

// ---------------------------------------------------------------------------
// End of El-Torito boot functions
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Start of int13 when emulating a device from the cd
// ---------------------------------------------------------------------------

void BIOSCALL int13_cdemu(disk_regs_t r)
{
    // @TODO: a macro or a function for getting the EBDA segment
    uint16_t        ebda_seg=read_word(0x0040,0x000E);
    uint8_t         device, status;
    uint16_t        vheads, vspt, vcylinders;
    uint16_t        head, sector, cylinder, nbsectors;
    uint32_t        vlba, ilba, slba, elba;
    uint16_t        before, segment, offset;
    uint8_t         atacmd[12];
    cdemu_t __far   *cdemu;

    cdemu = ebda_seg :> &EbdaData->cdemu;

    BX_DEBUG_INT13_ET("%s: AX=%04x BX=%04x CX=%04x DX=%04x ES=%04x\n", __func__, AX, BX, CX, DX, ES);

    /* at this point, we are emulating a floppy/harddisk */
    
    // Recompute the device number
    device  = cdemu->controller_index * 2;
    device += cdemu->device_spec;
    
    SET_DISK_RET_STATUS(0x00);
    
    /* basic checks : emulation should be active, dl should equal the emulated drive */
    if( (cdemu->active ==0 )
      || (cdemu->emulated_drive != GET_DL())) {
        BX_INFO("%s: function %02x, emulation not active for DL= %02x\n", __func__, GET_AH(), GET_DL());
        goto int13_fail;
    }
    
    switch (GET_AH()) {

    // all those functions return SUCCESS
    case 0x00: /* disk controller reset */
    case 0x09: /* initialize drive parameters */
    case 0x0c: /* seek to specified cylinder */
    case 0x0d: /* alternate disk reset */  // FIXME ElTorito Various. should really reset ?
    case 0x10: /* check drive ready */     // FIXME ElTorito Various. should check if ready ?
    case 0x11: /* recalibrate */
    case 0x14: /* controller internal diagnostic */
    case 0x16: /* detect disk change */
        goto int13_success;
        break;

    // all those functions return disk write-protected
    case 0x03: /* write disk sectors */
    case 0x05: /* format disk track */
        SET_AH(0x03);
        goto int13_fail_noah;
        break;

    case 0x01: /* read disk status */
        status=read_byte(0x0040, 0x0074);
        SET_AH(status);
        SET_DISK_RET_STATUS(0);
        
        /* set CF if error status read */
        if (status)
            goto int13_fail_nostatus;
        else
            goto int13_success_noah;
        break;

    case 0x02: // read disk sectors
    case 0x04: // verify disk sectors
        vspt       = cdemu->vdevice.spt;
        vcylinders = cdemu->vdevice.cylinders;
        vheads     = cdemu->vdevice.heads;
        ilba       = cdemu->ilba;
        
        sector    = GET_CL() & 0x003f;
        cylinder  = (GET_CL() & 0x00c0) << 2 | GET_CH();
        head      = GET_DH();
        nbsectors = GET_AL();
        segment   = ES;
        offset    = BX;
        
        // no sector to read ?
        if(nbsectors==0)
            goto int13_success;
        
        // sanity checks sco openserver needs this!
        if ((sector   >  vspt)
          || (cylinder >= vcylinders)
          || (head     >= vheads)) {
            goto int13_fail;
        }
        
        // After validating the input, verify does nothing
        if (GET_AH() == 0x04)
            goto int13_success;
        
        segment = ES+(BX / 16);
        offset  = BX % 16;
        
        // calculate the virtual lba inside the image
        vlba=((((uint32_t)cylinder*(uint32_t)vheads)+(uint32_t)head)*(uint32_t)vspt)+((uint32_t)(sector-1));
        
        // In advance so we don't lose the count
        SET_AL(nbsectors);
        
        // start lba on cd
        slba  = (uint32_t)vlba/4;
        before= (uint32_t)vlba%4;
        
        // end lba on cd
        elba = (uint32_t)(vlba+nbsectors-1)/4;
        
        _fmemset(&atacmd,0,12);
        atacmd[0]=0x28;                      // READ command
        atacmd[7]=((uint16_t)(elba-slba+1) & 0xff00) >> 8; // Sectors
        atacmd[8]=((uint16_t)(elba-slba+1) & 0x00ff);      // Sectors
        atacmd[2]=(ilba+slba & 0xff000000) >> 24;  // LBA
        atacmd[3]=(ilba+slba & 0x00ff0000) >> 16;
        atacmd[4]=(ilba+slba & 0x0000ff00) >> 8;
        atacmd[5]=(ilba+slba & 0x000000ff);
        if((status = ata_cmd_packet(device, 12, &atacmd, before*512, nbsectors*512L, ATA_DATA_IN, MK_FP(segment,offset))) != 0) {
            BX_INFO("%s: function %02x, error %02x !\n", __func__, GET_AH(), status);
            SET_AH(0x02);
            SET_AL(0);
            goto int13_fail_noah;
        }
        
        goto int13_success;
        break;

    case 0x08: /* read disk drive parameters */
        vspt       = cdemu->vdevice.spt;
        vcylinders = cdemu->vdevice.cylinders - 1;
        vheads     = cdemu->vdevice.heads - 1;
        
        SET_AL( 0x00 );
        SET_BL( 0x00 );
        SET_CH( vcylinders & 0xff );
        SET_CL((( vcylinders >> 2) & 0xc0) | ( vspt  & 0x3f ));
        SET_DH( vheads );
        SET_DL( 0x02 );   // FIXME ElTorito Various. should send the real count of drives 1 or 2
                          // FIXME ElTorito Harddisk. should send the HD count

        switch(cdemu->media) {
        case 0x01: SET_BL( 0x02 ); break;
        case 0x02: SET_BL( 0x04 ); break;
        case 0x03: SET_BL( 0x06 ); break;
        }

        DI = (uint16_t)&diskette_param_table;   // @todo: or really DPT2?
        ES = 0xF000;                            // @todo: how to make this relocatable?
        goto int13_success;
        break;

    case 0x15: /* read disk drive size */
        // FIXME ElTorito Harddisk. What geometry to send ?
        SET_AH(0x03);
        goto int13_success_noah;
        break;

    // all those functions return unimplemented
    case 0x0a: /* read disk sectors with ECC */
    case 0x0b: /* write disk sectors with ECC */
    case 0x18: /* set media type for format */
    case 0x41: // IBM/MS installation check
      // FIXME ElTorito Harddisk. Darwin would like to use EDD
    case 0x42: // IBM/MS extended read
    case 0x43: // IBM/MS extended write
    case 0x44: // IBM/MS verify sectors
    case 0x45: // IBM/MS lock/unlock drive
    case 0x46: // IBM/MS eject media
    case 0x47: // IBM/MS extended seek
    case 0x48: // IBM/MS get drive parameters
    case 0x49: // IBM/MS extended media change
    case 0x4e: // ? - set hardware configuration
    case 0x50: // ? - send packet command
    default:
        BX_INFO("%s: function AH=%02x unsupported, returns fail\n", __func__, GET_AH());
        goto int13_fail;
        break;
    }

int13_fail:
    SET_AH(0x01); // defaults to invalid function in AH or invalid parameter
int13_fail_noah:
    SET_DISK_RET_STATUS(GET_AH());
int13_fail_nostatus:
    SET_CF();     // error occurred
    return;

int13_success:
    SET_AH(0x00); // no error
int13_success_noah:
    SET_DISK_RET_STATUS(0x00);
    CLEAR_CF();   // no error
    return;
}

// ---------------------------------------------------------------------------
// Start of int13 for cdrom
// ---------------------------------------------------------------------------

void BIOSCALL int13_cdrom(uint16_t EHBX, disk_regs_t r)
{
    uint16_t        ebda_seg=read_word(0x0040,0x000E);
    uint8_t         device, status, locks;
    uint8_t         atacmd[12];
    uint32_t        lba;
    uint16_t        count, segment, offset, size;
    ata_t __far     *ata;

    ata = ebda_seg :> &EbdaData->ata;

    
    BX_DEBUG_INT13_CD("%s: AX=%04x BX=%04x CX=%04x DX=%04x ES=%04x\n", __func__, AX, BX, CX, DX, ES);
    
    SET_DISK_RET_STATUS(0x00);
    
    /* basic check : device should be 0xE0+ */
    if( (GET_ELDL() < 0xE0) || (GET_ELDL() >= 0xE0+BX_MAX_ATA_DEVICES) ) {
        BX_DEBUG("%s: function %02x, ELDL out of range %02x\n", __func__, GET_AH(), GET_ELDL());
        goto int13_fail;
    }
    
    // Get the ata channel
    device = ata->cdidmap[GET_ELDL()-0xE0];
    
    /* basic check : device has to be valid  */
    if (device >= BX_MAX_ATA_DEVICES) {
        BX_DEBUG("%s: function %02x, unmapped device for ELDL=%02x\n", __func__, GET_AH(), GET_ELDL());
        goto int13_fail;
    }

  switch (GET_AH()) {

    // all those functions return SUCCESS
    case 0x00: /* disk controller reset */
    case 0x09: /* initialize drive parameters */
    case 0x0c: /* seek to specified cylinder */
    case 0x0d: /* alternate disk reset */
    case 0x10: /* check drive ready */
    case 0x11: /* recalibrate */
    case 0x14: /* controller internal diagnostic */
    case 0x16: /* detect disk change */
      goto int13_success;
      break;

    // all those functions return disk write-protected
    case 0x03: /* write disk sectors */
    case 0x05: /* format disk track */
    case 0x43: // IBM/MS extended write
      SET_AH(0x03);
      goto int13_fail_noah;
      break;

    case 0x01: /* read disk status */
      status = read_byte(0x0040, 0x0074);
      SET_AH(status);
      SET_DISK_RET_STATUS(0);

      /* set CF if error status read */
      if (status) goto int13_fail_nostatus;
      else        goto int13_success_noah;
      break;

    case 0x15: /* read disk drive size */
      SET_AH(0x02);
      goto int13_fail_noah;
      break;

    case 0x41: // IBM/MS installation check
      BX=0xaa55;     // install check
      SET_AH(0x30);  // EDD 2.1
      CX=0x0007;     // ext disk access, removable and edd
      goto int13_success_noah;
      break;

    case 0x42: // IBM/MS extended read
    case 0x44: // IBM/MS verify sectors
    case 0x47: // IBM/MS extended seek

      count=read_word(DS, SI+(uint16_t)&Int13Ext->count);
      segment=read_word(DS, SI+(uint16_t)&Int13Ext->segment);
      offset=read_word(DS, SI+(uint16_t)&Int13Ext->offset);

      // Can't use 64 bits lba
      lba=read_dword(DS, SI+(uint16_t)&Int13Ext->lba2);
      if (lba != 0L) {
        BX_PANIC("%s: function %02x. Can't use 64bits lba\n", __func__,GET_AH());
        goto int13_fail;
        }

      // Get 32 bits lba
      lba=read_dword(DS, SI+(uint16_t)&Int13Ext->lba1);

      // If verify or seek
      if (( GET_AH() == 0x44 ) || ( GET_AH() == 0x47 ))
        goto int13_success;

      _fmemset(&atacmd,0,12);
      atacmd[0]=0x28;                      // READ command
      atacmd[7]=(count & 0xff00) >> 8;     // Sectors
      atacmd[8]=(count & 0x00ff);          // Sectors
      atacmd[2]=(lba & 0xff000000) >> 24;  // LBA
      atacmd[3]=(lba & 0x00ff0000) >> 16;
      atacmd[4]=(lba & 0x0000ff00) >> 8;
      atacmd[5]=(lba & 0x000000ff);
      status = ata_cmd_packet(device, 12, &atacmd, 0, count*2048L, ATA_DATA_IN, MK_FP(segment,offset));

      count = (uint16_t)(ata->trsfbytes >> 11);
      write_word(DS, SI+(uint16_t)&Int13Ext->count, count);

      if (status != 0) {
        BX_INFO("%s: function %02x, status %02x !\n", __func__, GET_AH(), status);
        SET_AH(0x0c);
        goto int13_fail_noah;
        }

      goto int13_success;
      break;

    case 0x45: // IBM/MS lock/unlock drive
      if (GET_AL() > 2) goto int13_fail;

      locks = ata->devices[device].lock;

      switch (GET_AL()) {
        case 0 :  // lock
          if (locks == 0xff) {
            SET_AH(0xb4);
            SET_AL(1);
            goto int13_fail_noah;
            }
          ata->devices[device].lock = ++locks;
          SET_AL(1);
          break;
        case 1 :  // unlock
          if (locks == 0x00) {
            SET_AH(0xb0);
            SET_AL(0);
            goto int13_fail_noah;
            }
          ata->devices[device].lock = --locks;
          SET_AL(locks==0?0:1);
          break;
        case 2 :  // status
          SET_AL(locks==0?0:1);
          break;
        }
      goto int13_success;
      break;

    case 0x46: // IBM/MS eject media
      locks = ata->devices[device].lock;

      if (locks != 0) {
        SET_AH(0xb1); // media locked
        goto int13_fail_noah;
        }
      // FIXME should handle 0x31 no media in device
      // FIXME should handle 0xb5 valid request failed

#if 0 //@todo: implement!
      // Call removable media eject
      ASM_START
        push bp
        mov  bp, sp

        mov ah, #0x52
        int #0x15
        mov _int13_cdrom.status + 2[bp], ah
        jnc int13_cdrom_rme_end
        mov _int13_cdrom.status, #1
int13_cdrom_rme_end:
        pop bp
      ASM_END
#endif

      if (status != 0) {
        SET_AH(0xb1); // media locked
        goto int13_fail_noah;
      }

      goto int13_success;
      break;

    case 0x48: // IBM/MS get drive parameters
      size = read_word(DS,SI+(uint16_t)&Int13Ext->size);

      // Buffer is too small
      if(size < 0x1a)
        goto int13_fail;

      // EDD 1.x
      if(size >= 0x1a) {
        uint16_t   blksize;

        blksize   = ata->devices[device].blksize;

        write_word(DS, SI+(uint16_t)&Int13DPT->size, 0x1a);
        write_word(DS, SI+(uint16_t)&Int13DPT->infos, 0x74); // removable, media change, lockable, max values
        write_dword(DS, SI+(uint16_t)&Int13DPT->cylinders, 0xffffffff);
        write_dword(DS, SI+(uint16_t)&Int13DPT->heads, 0xffffffff);
        write_dword(DS, SI+(uint16_t)&Int13DPT->spt, 0xffffffff);
        write_dword(DS, SI+(uint16_t)&Int13DPT->sector_count1, 0xffffffff);  // FIXME should be Bit64
        write_dword(DS, SI+(uint16_t)&Int13DPT->sector_count2, 0xffffffff);
        write_word(DS, SI+(uint16_t)&Int13DPT->blksize, blksize);
        }

      // EDD 2.x
      if(size >= 0x1e) {
        uint8_t  channel, irq, mode, checksum, i;
        uint16_t iobase1, iobase2, options;

        write_word(DS, SI+(uint16_t)&Int13DPT->size, 0x1e);

        write_word(DS, SI+(uint16_t)&Int13DPT->dpte_segment, ebda_seg);
        write_word(DS, SI+(uint16_t)&Int13DPT->dpte_offset, (uint16_t)&EbdaData->ata.dpte);

        // Fill in dpte
        channel = device / 2;
        iobase1 = ata->channels[channel].iobase1;
        iobase2 = ata->channels[channel].iobase2;
        irq     = ata->channels[channel].irq;
        mode    = ata->devices[device].mode;

        // FIXME atapi device
        options  = (1<<4); // lba translation
        options |= (1<<5); // removable device
        options |= (1<<6); // atapi device
        options |= (mode==ATA_MODE_PIO32?1:0<<7);

        ata->dpte.iobase1   = iobase1;
        ata->dpte.iobase2   = iobase2;
        ata->dpte.prefix    = (0xe | (device % 2))<<4;
        ata->dpte.unused    = 0xcb;
        ata->dpte.irq       = irq;
        ata->dpte.blkcount  = 1 ;
        ata->dpte.dma       = 0;
        ata->dpte.pio       = 0;
        ata->dpte.options   = options;
        ata->dpte.reserved  = 0;
        ata->dpte.revision  = 0x11;

        checksum=0;
        for (i=0; i<15; i++) checksum += read_byte(ebda_seg, (uint16_t)&EbdaData->ata.dpte + i);
        checksum = -checksum;
        ata->dpte.checksum = checksum;
        }

      // EDD 3.x
      if(size >= 0x42) {
        uint8_t  channel, iface, checksum, i;
        uint16_t iobase1;

        channel = device / 2;
        iface   = ata->channels[channel].iface;
        iobase1 = ata->channels[channel].iobase1;

        write_word(DS, SI+(uint16_t)&Int13DPT->size, 0x42);
        write_word(DS, SI+(uint16_t)&Int13DPT->key, 0xbedd);
        write_byte(DS, SI+(uint16_t)&Int13DPT->dpi_length, 0x24);
        write_byte(DS, SI+(uint16_t)&Int13DPT->reserved1, 0);
        write_word(DS, SI+(uint16_t)&Int13DPT->reserved2, 0);

        if (iface==ATA_IFACE_ISA) {
          write_byte(DS, SI+(uint16_t)&Int13DPT->host_bus[0], 'I');
          write_byte(DS, SI+(uint16_t)&Int13DPT->host_bus[1], 'S');
          write_byte(DS, SI+(uint16_t)&Int13DPT->host_bus[2], 'A');
          write_byte(DS, SI+(uint16_t)&Int13DPT->host_bus[3], 0);
          }
        else {
          // FIXME PCI
          }
        write_byte(DS, SI+(uint16_t)&Int13DPT->iface_type[0], 'A');
        write_byte(DS, SI+(uint16_t)&Int13DPT->iface_type[1], 'T');
        write_byte(DS, SI+(uint16_t)&Int13DPT->iface_type[2], 'A');
        write_byte(DS, SI+(uint16_t)&Int13DPT->iface_type[3], 0);

        if (iface==ATA_IFACE_ISA) {
          write_word(DS, SI+(uint16_t)&Int13DPT->iface_path[0], iobase1);
          write_word(DS, SI+(uint16_t)&Int13DPT->iface_path[2], 0);
          write_dword(DS, SI+(uint16_t)&Int13DPT->iface_path[4], 0L);
          }
        else {
          // FIXME PCI
          }
        write_byte(DS, SI+(uint16_t)&Int13DPT->device_path[0], device%2);
        write_byte(DS, SI+(uint16_t)&Int13DPT->device_path[1], 0);
        write_word(DS, SI+(uint16_t)&Int13DPT->device_path[2], 0);
        write_dword(DS, SI+(uint16_t)&Int13DPT->device_path[4], 0L);

        checksum=0;
        for (i=30; i<64; i++) checksum+=read_byte(DS, SI + i);
        checksum = -checksum;
        write_byte(DS, SI+(uint16_t)&Int13DPT->checksum, checksum);
        }

      goto int13_success;
      break;

    case 0x49: // IBM/MS extended media change
      // always send changed ??
      SET_AH(06);
      goto int13_fail_nostatus;
      break;

    case 0x4e: // // IBM/MS set hardware configuration
      // DMA, prefetch, PIO maximum not supported
      switch (GET_AL()) {
        case 0x01:
        case 0x03:
        case 0x04:
        case 0x06:
          goto int13_success;
          break;
        default :
          goto int13_fail;
        }
      break;

    // all those functions return unimplemented
    case 0x02: /* read sectors */
    case 0x04: /* verify sectors */
    case 0x08: /* read disk drive parameters */
    case 0x0a: /* read disk sectors with ECC */
    case 0x0b: /* write disk sectors with ECC */
    case 0x18: /* set media type for format */
    case 0x50: // ? - send packet command
    default:
      BX_INFO("%s: unsupported AH=%02x\n", __func__, GET_AH());
      goto int13_fail;
      break;
    }

int13_fail:
    SET_AH(0x01); // defaults to invalid function in AH or invalid parameter
int13_fail_noah:
    SET_DISK_RET_STATUS(GET_AH());
int13_fail_nostatus:
    SET_CF();     // error occurred
    return;

int13_success:
    SET_AH(0x00); // no error
int13_success_noah:
    SET_DISK_RET_STATUS(0x00);
    CLEAR_CF();   // no error
    return;
}

// ---------------------------------------------------------------------------
// End of int13 for cdrom
// ---------------------------------------------------------------------------