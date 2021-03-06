#!/usr/bin/env runhaskell
{-# OPTIONS_GHC -Wall -fno-warn-missing-signatures #-}
{-# LANGUAGE RecordWildCards #-}
module Main where

import Control.Monad
import Development.Shake
import Development.Shake.AVR
import Development.Shake.FilePath

srcDir          = "src"
docDir          = "doc/html"
doxyfile        = "Doxyfile"
buildRoot       = "build"
proj            = "vTree"

avrdudeFlags    = ["-c", "dragon_pdi"]

cFlags Board{..} = ["-Wall", "-Os", "-std=c99",
    "-Iinclude", "-Ipt-1.4", "-I" ++ srcDir </> "board" </> boardName,
    "-DF_CPU=" ++ show (round boardClock :: Integer) ++ "UL",
    "-mmcu=" ++ boardDevice]

ldFlags = ["-Wl,--gc-sections,--relax"]

data Board = Board
    { boardName     :: String
    , boardDevice   :: String
    , boardClock    :: Rational
    }

replaceDirAndExt dir ext = flip replaceDirectory dir . flip replaceExtension ext

-- rules for a single board:
-- compile common sources from src/*.c and board
-- sources from src/board/<board>/*.c
boardRules board@Board{..} = do
    -- board-specific file names and paths
    let projBase = proj ++ "-" ++ boardName
        
        boardDir        = srcDir </> "board" </> boardName
        buildDir        = buildRoot </> boardName
        
        hex = projBase <.> "hex"
        elf = buildRoot </> projBase <.> "elf"
    
    want [hex]
    
    -- to build the hex file, first build the elf an then objcopy it.
    -- run avr_size on the elf, too, because I like to see where
    -- things stand.
    hex *> \out -> do
        avr_size elf
        avr_objcopy "ihex" ["-j", ".text", "-j", ".data"] elf out
    
    -- to build the elf file, build an object file for every source file
    -- in src/ and src/board/<board>/, then link those.
    elf *> \out -> do
        srcs        <- getDirectoryFiles srcDir   ["*.c"]
        boardSrcs   <- getDirectoryFiles boardDir ["*.c"]
        let objs = map (replaceDirAndExt buildDir "o") (srcs ++ boardSrcs)
        avr_ld' "avr-gcc" (cFlags board ++ ldFlags) objs out
    
    -- to build an object file, find the source (searching src/ first,
    -- then src/board/<board>/) and compile it.
    buildDir </> "*.o" *> \out -> do
        let commonSrc = replaceDirAndExt srcDir   "c" out
            boardSrc  = replaceDirAndExt boardDir "c" out
        
        isCommon <- doesFileExist commonSrc
        let input = if isCommon then commonSrc else boardSrc
        
        avr_gcc (cFlags board) input out
    
    return hex

main = shakeArgs shakeOptions $ do
    "clean" ~> do
        removeFilesAfter "." ["vTree-*.hex"]
        
        buildExists <- doesDirectoryExist buildRoot
        when buildExists $ removeFilesAfter buildRoot ["//*"]
        
        docExists <- doesDirectoryExist docDir
        when docExists $ removeFilesAfter docDir ["//*"]
    
    "doc" ~> need ["doc/html/index.html"]
    "doc/html/index.html" *> \_ -> do
        need [doxyfile]
        need ["README.md"]
        need . map (srcDir </>) =<< getDirectoryFiles srcDir ["//*.c"]
        command_ [] "doxygen" [doxyfile]
    
    -- note that "flash" only works for xmega
    -- (the teensy protocol isn't supported by avrdude)
    "flash" ~> avrdude "atxmega8e5" avrdudeFlags (w Flash "vTree-xmega.hex")
    
    mapM_ boardRules
        [ Board "teensy" "atmega32u4" 16e6
        , Board "xmega"  "atxmega8e5" 32e6
        ]
