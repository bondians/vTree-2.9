#!/usr/bin/env runhaskell
module Main where

import Control.Monad
import Development.Shake
import Development.Shake.AVR
import Development.Shake.FilePath

srcDir          = "src"
buildRoot       = "build"
proj            = "vTree"

avrdudeFlags    = ["-c", "dragon_pdi", "-p", "atxmega8e5"]
usbPort         = "usb"

cFlags board = ["-Wall", "-Os", "-std=c99",
    "-Iinclude",
    "-DF_CPU=" ++ show (boardClock board) ++ "UL",
    "-mmcu=" ++ boardDevice board]

data Board = Board
    { boardName     :: String
    , boardDevice   :: String
    , boardClock    :: Integer
    }

boards =
    [ Board "teensy" "atmega32u4" (round 16e6)
    , Board "xmega"  "atxmega8e5" (round 32e6)
    ]

main = shakeArgs shakeOptions $ do
    want =<< mapM boardRules boards
    
    phony "clean" $ do
        removeFilesAfter "." ["*.hex", "*.elf"]
        sequence_
            [ removeFilesAfter (buildRoot </> boardName board) ["*.o"]
            | board <- boards
            ]
    
    phony "flash" $ do -- only works for xmega
        avrdude "flash" avrdudeFlags "vTree-xmega.hex" usbPort
    
    "*.hex" *> \out -> do
        let elf = out `replaceExtension` "elf"
        avr_objcopy "ihex" ["-j", ".text", "-j", ".data"] elf out

boardRules board = do
    let projBase = proj ++ "-" ++ boardName board
        
        boardDir        = srcDir </> "board" </> boardName board
        buildDir        = buildRoot </> boardName board
        boardBuildDir   = buildDir </> "board"
        
        hex = projBase <.> "hex"
        elf = projBase <.> "elf"
    
    elf *> \out -> do
        srcs        <- getDirectoryFiles srcDir   ["*.c"]
        boardSrcs   <- getDirectoryFiles boardDir ["*.c"]
        let objs = [src `replaceDirectory` buildDir      `replaceExtension` "o" | src <- srcs]
                ++ [src `replaceDirectory` boardBuildDir `replaceExtension` "o" | src <- boardSrcs]
        avr_ld' "avr-gcc" (cFlags board) objs out
    
    buildDir </> "*.o" *> \out -> do
        let input = out `replaceDirectory` srcDir `replaceExtension` "c"
        avr_gcc (cFlags board) input out
    
    boardBuildDir </> "*.o" *> \out -> do
        let input = out `replaceDirectory` boardDir `replaceExtension` "c"
        avr_gcc (cFlags board) input out
    
    return hex