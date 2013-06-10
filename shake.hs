#!/usr/bin/env runhaskell
module Main where

import Development.Shake
import Development.Shake.AVR
import Development.Shake.FilePath

srcDir          = "src"
buildDir        = "build"
proj            = "vTree"

device          = "attiny2313a"
clock           = round 16e6

avrdudeFlags    = ["-c", "dragon_isp", "-p", device]
usbPort         = "usb"

cFlags = ["-Wall", "-Os", "-std=c99",
    "-Iinclude",
    "-DF_CPU=" ++ show clock ++ "UL",
    "-mmcu=" ++ device]

main = shakeArgs shakeOptions $ do
    let hex = proj <.> "hex"
        bin = proj <.> "bin"
    want [hex, bin]
    
    phony "clean" $ do
        removeFilesAfter "." ["*.hex", buildDir </> "*.o", buildDir </> "*.elf"]
    
    phony "flash" $ do
        avrdude avrdudeFlags hex usbPort
    
    buildDir </> "vTree.elf" *> \out -> do
        srcs <- getDirectoryFiles srcDir ["*.c"]
        let objs = [src `replaceDirectory` buildDir `replaceExtension` "o" | src <- srcs]
        avr_ld' "avr-gcc" cFlags objs out
    
    "*.hex" *> \out -> do
        let elf = out `replaceDirectory` buildDir `replaceExtension` "elf"
        avr_objcopy "ihex" ["-j", ".text", "-j", ".data"] elf out
    
    "*.bin" *> \out -> do
        let elf = out `replaceDirectory` buildDir `replaceExtension` "elf"
        avr_objcopy "binary" ["-j", ".text", "-j", ".data"] elf out
    
    buildDir </> "*.o" *> \out -> do
        let input = out `replaceDirectory` srcDir `replaceExtension` "c"
        avr_gcc cFlags input out
