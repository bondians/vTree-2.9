{-# LANGUAGE ViewPatterns #-}
module Linearize where

import Data.Colour.RGBSpace
import Data.Colour.RGBSpace.HSV

-- compute the duty cycles required to achieve a target color
linearize orig@(RGB r g b)
    | r > g     = case linearize (RGB g r b) of RGB g' r' b' -> RGB r' g' b'
    | g > b     = case linearize (RGB r b g) of RGB r' b' g' -> RGB r' g' b'
    | r == b    = orig
    | otherwise = RGB (3 * r * x) ((r + 2 * g) * x) b
        where
            x = b / (r + g + b)

-- compute the color displayed when channels are set to given duty cycles
delinearize (RGB r g b)
    | r > g     = case delinearize (RGB g r b) of RGB g' r' b' -> RGB r' g' b'
    | g > b     = case delinearize (RGB r b g) of RGB r' b' g' -> RGB r' g' b'
    | r == b    = RGB r g b
    | otherwise = RGB (r' * x) (g' * x) b
        where
            r' = r / 3
            g' = r' + (g - r) / 2
            b' = g' + (b - g)
            
            x = b / b'

-- observations:
-- all errors appear to be completely independent of "value" (brightness)
-- all errors appear as shifts toward the nearest primary.
-- overall the delinearized colors are "punchier" 
--      - more saturated, hue-shifted slightly toward primaries
-- hue error appears to additionally be independent of saturation
--      - it only depends on target hue.
-- primaries and secondaries (hue = 60n) have 0 hue error.
-- tertiaries (hue = 60n + 30) have worst hue error, about 10 degrees.
--      note that peak errors are not exactly at the tertiaries, they're
--      about 5 degrees toward the nearest secondary (e.g, peaks at
--      35, 85, etc.).
-- saturation error appears to depend on both hue and saturation.
-- saturation error is most severe for primaries (peaking at about
--      0.268 at or near s=1/e), mildest for tertiaries (peaking at
--      about 0.1 at or near s = 0.4).
-- the chosen normalization appears correct; value error is uniformly 0.
hsvError c = (h1 - h0, s1 - s0, v1 - v1)
    where
        (h0, s0, v0) = hsvView c
        (h1, s1, v1) = hsvView (delinearize c)
        