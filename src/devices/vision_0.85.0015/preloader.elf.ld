OUTPUT_ARCH(arm)
ENTRY(entrypoint_preloader)

PHDRS 
{ 
    text PT_LOAD FLAGS(7); 
    bss PT_LOAD FLAGS(7);
}

SECTIONS
{
    . = 0x058D3000;
    .text : 
    { 
        *(.text)
    } :text =0xDEADC0DE

    .bss :
    {
        *(.bss) *(.data) *(.rodata*)
    } :bss

    PROVIDE(__memcpy = 0x8D023018);
    PROVIDE(__memset = 0x8D022FF8);
    PROVIDE(__fb_cmd_oem = 0x8D0020C8);
    PROVIDE(__setting_set_bootmode = 0x8D008638);
    PROVIDE(__do_reboot = 0x8D00AE0C);
    PROVIDE(__usb_send = 0x8D001F24);
    PROVIDE(__usb_send_to_device_1 = 0x8D051064);
    PROVIDE(__strtol16 = 0x8D0235B4);
    PROVIDE(__strlen = 0x8D022E18);

    /DISCARD/ :
    {
        *(.pdr);
        *(.gnu.attributes);
        *(.reginfo);
        *(.note);
        *(.comment);
        *(__ex_table);
        *(interp);
    }
}
