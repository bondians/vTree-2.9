module BOM where

import Control.Applicative
import Control.Monad.State
import Control.Monad.RWS
import Control.Monad.Logic
import Data.Function
import Data.List
import Data.Monoid
import Data.Ord
import qualified Data.Map as M

-- TODO: check antenna connector genders for compatibility
bom rp =
    [ (1, pcb)
    
    , (1, regulator)
    , (1, inputCap)
    , (1, outputCap)
    , (1, filterBead)
    
    , (3, gateLimResistor)
    , (3, gatePullDownResistor)
    , (1, redVtgMatchResistor)
    , (3, mosfet)

    , (1, mcu)
    , (2, decouplingCapacitor)
    , (1, resetPullUpResistor)
    
    , (1, xbee)
    , (2, xbeeHeader)
    
    , (1, antennaBulkhead rp)
    , (1, antenna rp)
    ]

pcb = oshParkPCB 0.8991 "xmega e5 target board"

mcu = basicPart mouser "ATXMEGA8E5-AU"
    [ (1,   1.83)
    , (10,  1.53)
    , (25,  1.15)
    , (100, 1.04)
    ] ++ basicPart digikey "ATXMEGA8E5-AU"
    [ (1,   1.83)
    , (25,  1.1476)
    , (100, 1.02)
    ]

regulator = basicPart mouser "LD2981ABU33TR"
    [ (1,    0.69)
    , (10,   0.61)
    , (100,  0.466)
    , (250,  0.406)
    , (500,  0.346)
    , (1000, 0.277)
    , (2000, 0.243)
    ] ++ basicPart mouser "L78L33ABUTR"
    [ (1, 0.59)
    , (10, 0.47)
    , (100, 0.321)
    , (250, 0.266)
    , (500, 0.219)
    , (1000, 0.164)
    ] ++ basicPart digikey "L78L33ABUTR"
    [ (1, 0.56)
    , (10, 0.441)
    , (25, 0.372)
    , (100, 0.3032)
    , (250, 0.25116)
    , (500, 0.20748)
    , (1000, 0.1554)
    ]

inputCap = basicPart mouser "AVE107M16X16T-F"
    [ (1,   0.12)
    , (50,  0.11)
    , (100, 0.092)
    , (500, 0.075)
    ] ++ basicPart digikey "UWZ1C101MCL1GB"
    [ (1, 0.45)
    , (10, 0.391)
    , (25, 0.2796)
    , (50, 0.2376)
    , (100, 0.195)
    , (250, 0.15652)
    , (500, 0.14254)
    ]

outputCap = basicPart mouser "AVE476M06C12T-F"
    [ (1,   0.1)
    , (50,  0.078)
    , (100, 0.075)
    , (500, 0.061)
    ] ++ basicPart digikey "UWX0G470MCL1GB"
    [ (1, 0.28)
    , (10, 0.235)
    , (25, 0.1616)
    , (50, 0.1324)
    , (100, 0.1176)
    , (250, 0.0882)
    , (500, 0.07792)
    , (1000, 0.06909)
    ]

gateLimResistor = basicPart mouser "ERJ-2GEJ151X"
    [ (1,    0.08)
    , (10,   0.012)
    , (100,  0.005)
    , (1000, 0.004)
    ] ++ basicPart digikey "RC1005J161CS"
    [ (1, 0.10)
    , (10, 0.013)
    , (25, 0.0092)
    , (50, 0.007)
    , (100, 0.0051)
    , (250, 0.00392)
    , (500, 0.00312)
    , (1000, 0.00230)
    , (2500, 0.002)
    ]

redVtgMatchResistor = basicPart mouser "283-15-RC"
    [ (1,      0.21)
    , (100,    0.145)
    , (1000,   0.09)
    , (5000,   0.065)
    , (10000,  0.055)
    , (25000,  0.045)
    , (50000,  0.037)
    , (100000, 0.03)
    , (250000, 0.027)
    ] ++ basicPart mouser "MOS3CT631R150J"
    [ (1,      0.20)
    , (100,    0.153)
    , (500,    0.128)
    ] ++ basicPart digikey "PNP300JR-73-15R"
    [ (1,    0.45)
    , (10,   0.391)
    , (25,   0.2792)
    , (100,  0.1794)
    , (250,  0.13956)
    , (500,  0.10528)
    , (1000, 0.08135)
    ]

gatePullDownResistor = basicPart mouser "ERJ-2GEJ104X"
    [ (1,    0.08)
    , (10,   0.012)
    , (100,  0.005)
    , (1000, 0.004)
    ] ++ basicPart digikey "RC1005J753CS"
    [ (1, 0.10)
    , (10, 0.013)
    , (25, 0.0092)
    , (50, 0.007)
    , (100, 0.0051)
    , (250, 0.00392)
    , (500, 0.00312)
    , (1000, 0.0023)
    , (2500, 0.002)
    ]

mosfet = basicPart mouser "SI2302CDS-T1-E3"
    [ (1,    0.15)
    , (10,   0.14)
    , (100,  0.13)
    , (250,  0.125)
    , (500,  0.12)
    , (1000, 0.115)
    ] ++ basicPart digikey "SI2302CDS-T1-E3"
    [ (1, 0.51)
    , (25, 0.2976)
    , (100, 0.24)
    , (250, 0.1728)
    , (500, 0.1392)
    , (1000, 0.108)
    ] ++ basicPart digikey "IRFML8244TRPBF"
    [ (1, 0.38)
    , (10, 0.213)
    , (25, 0.1832)
    , (50, 0.1542)
    , (100, 0.138)
    , (250, 0.12544)
    , (500, 0.11286)
    , (1000, 0.10565)
    ]

filterBead = basicPart mouser "436-0102-RC"
    [ (1,      0.05)
    , (100,    0.047)
    , (500,    0.039)
    , (1000,   0.038)
    , (4000,   0.03)
    , (8000,   0.029)
    , (20000,  0.026)
    , (40000,  0.025)
    , (100000, 0.024)
    ] ++ basicPart digikey "BLM18AG102SN1D"
    [ (1, 0.1)
    , (10, 0.079)
    , (50, 0.0648)
    , (100, 0.0476)
    , (250, 0.0346)
    , (500, 0.0317)
    , (1000, 0.02738)
    ]

decouplingCapacitor = basicPart mouser "VJ0603Y104JXJPW1BC"
    [ (1,    0.06)
    , (50,   0.024)
    , (100,  0.019)
    , (500,  0.017)
    , (1000, 0.015)
    ] ++ basicPart digikey "CL10F104ZA8NNNC"
    [ (1, 0.1)
    , (10, 0.017)
    , (25, 0.012)
    , (50, 0.009)
    , (100, 0.0076)
    , (250, 0.00624)
    , (500, 0.00536)
    , (1000, 0.00417)
    ]

resetPullUpResistor = basicPart mouser "CRCW040210K0FKED"
    [ (1,    0.08)
    , (10,   0.044)
    , (100,  0.021)
    , (1000, 0.015)
    ] ++ basicPart digikey "1622826-4"
    [ (1, 0.1)
    , (10, 0.01)
    , (25, 0.0072)
    , (50, 0.0056)
    , (100, 0.0042)
    , (250, 0.00316)
    , (500, 0.00254)
    , (1000, 0.00187)
    , (2500, 0.00162)
    ]

xbee = basicPart mouser  "XB24-Z7UIT-004" [(1, 17.00)]
    ++ basicPart digikey "XB24-Z7UIT-004" [(1, 17.00)]

xbeeHeader = basicPart mouser "950510-6102-AR"
    [ (1,    1.50)
    , (25,   1.33)
    , (50,   1.20)
    , (100,  1.09)
    , (200,  1.00)
    , (500,  0.92)
    , (1000, 0.86)
    , (2000, 0.80)
    ] ++ basicPart mouser "M22-7131042"
    [ (1,     1.78)
    , (10,    1.37)
    , (100,   0.963)
    , (500,   0.88)
    , (1000,  0.797)
    , (2000,  0.715)
    , (5000,  0.701)
    , (10000, 0.688)
    ] ++ basicPart adafruit "366"
    [ (1, 1.00)
    ] ++ basicPart sparkFun "8272"
    [ (1,   1.00)
    , (10,  0.90)
    , (100, 0.80)
    ] ++ basicPart digikey "NPPN101BFCN-RC"
    [ (1, 1.11)
    , (10, 0.923)
    , (25, 0.8552)
    , (50, 0.7696)
    , (100, 0.684)
    , (250, 0.59852)
    , (500, 0.5472)
    , (1000, 0.4959)
    , (2500, 0.4446)
    ]

antennaBulkhead True = basicPart adafruit "852"
    [ (1,   3.95)
    , (10,  3.56)
    , (100, 3.16)
    ] ++ basicPart sparkFun "662"
    [ (1,   4.95)
    , (10,  4.46)
    , (100, 3.96)
    ] ++ basicPart mouser "080-0001"
    [ (1,    4.40)
    , (50,   4.10)
    , (100,  4.07)
    , (1000, 4.00)
    ] ++ basicPart digikey "MAF94118"
    [ (1, 4.25)
    , (25, 3.4)
    , (50, 2.89)
    , (100, 2.55)
    , (500, 2.38)
    ]

antennaBulkhead False = basicPart adafruit "851"
    [ (1,   3.95)
    , (10,  3.56)
    , (100, 3.16)
    ] ++ basicPart mouser "CAB.011"
    [ (1,    2.92)
    , (10,   2.87)
    , (20,   2.83)
    , (50,   2.78)
    , (100,  2.67)
    , (500,  2.43)
    , (1000, 2.09)
    ] ++ basicPart digikey "CAB.011"
    [ (1, 2.92)
    , (100, 2.7405)
    ]

antenna True = basicPart mouser "W1030"
    [ (1, 3.92)
    , (10, 3.35)
    , (100, 2.74)
    , (500, 2.22)
    , (1000, 1.88)
    , (2500, 1.78)
    , (5000, 1.76)
    ] ++ basicPart digikey "W1030"
    [ (1, 3.38)
    , (10, 3.24)
    , (25, 2.97)
    , (50, 2.835)
    , (100, 2.7)
    , (250, 2.36252)
    , (500, 2.295)
    , (1000, 1.9575)
    , (2500, 1.8225)
    ]

antenna False = basicPart mouser "W1010"
    [ (1, 3.92)
    , (10, 3.35)
    , (100, 2.74)
    , (500, 2.22)
    , (1000, 1.88)
    , (2500, 1.78)
    , (5000, 1.76)
    ] ++ basicPart digikey "W1010"
    [ (1, 3.38)
    , (10, 3.24)
    , (25, 2.97)
    , (50, 2.835)
    , (100, 2.7)
    , (250, 2.36252)
    , (500, 2.295)
    , (1000, 1.9575)
    , (2500, 1.8225)
    ]


---------------------------------------

data Supplier = Supplier
    { supplierName  :: String
    , shipping      :: Double -- TODO: [(Integer, Part)] -> Double
    } deriving (Eq, Ord, Read, Show)

adafruit = Supplier "Adafruit" 3.99
sparkFun = Supplier "SparkFun" 5.00
mouser   = Supplier "Mouser" 4.99
oshPark  = Supplier "OSH Park" 0
digikey  = Supplier "Digikey" 5.47
newark   = Supplier "Newark" 8.50

oshParkPCB sz boardName = 
    [ Part 
        { supplier      = oshPark
        , partNo        = boardName
        , minimumQty    = 3
        , increment     = 3
        , price         = sz * 5 / 3
        }
    , Part
        { supplier      = oshPark
        , partNo        = boardName
        , minimumQty    = 10 * ceiling (15 / sz)
        , increment     = 10
        , price         = sz
        }
    ]

data Part = Part
    { supplier      :: Supplier
    , partNo        :: String
    , minimumQty    :: Integer
    , increment     :: Integer
    , price         :: Double
    } deriving (Eq, Ord, Read, Show)

basicPart supp num breaks =
    [ Part supp num moq 1 p
    | (moq, p) <- breaks
    ]

---------------------------------------

unitCost withShipping bom qty = cost / fromIntegral qty
    where 
        cost    | withShipping  = pCost + sCost
                | otherwise     = pCost
        
        (_, pCost, sCost) = selectBOM bom qty

selectParts  bom = (\(a,_,_) -> a    ) . selectBOM bom
orderCost    bom = (\(_,b,c) -> b + c) . selectBOM bom
partsCost    bom = (\(_,b,_) -> b    ) . selectBOM bom
shippingCost bom = (\(_,_,c) ->     c) . selectBOM bom

selectBOM parts qty = (bom, sum partCosts, shippingCost)
    where
        totalCost ((_, x), Sum y) = sum x + y
        ((bom, partCosts), Sum shippingCost) = minimumBy (comparing totalCost) $
            map (\(a,b) -> (unzip a, b)) $ 
                observeAll $
                    (\x -> evalRWST x () M.empty) $
                        flip mapM parts $ \(count, part) -> do
                            selectPart part (count * qty)

selectPart parts qty = do
    let suppliers = nub (map supplier parts)
    selected <- map supplierName <$> filterM selectSupplier suppliers
    let selectedParts   = filter (flip elem selected . supplierName . supplier) parts
    if null selectedParts
        then empty
        else pure (selectPart' selectedParts qty)

selectPart' parts qty = ((actualQty part, part), extendedPrice part)
    where
        part                = minimumBy cmpParts parts
        
        extras        part  = max 0 (qty - minimumQty part)
        increments    part  = ceiling (fromIntegral (extras part) / fromIntegral (increment part))
        actualQty     part  = minimumQty part + increments part * increment part
        extendedPrice part  = price part * fromIntegral (actualQty part)
        
        -- minimize price, break ties by maximizing qty
        cmpParts = mconcat
            [ comparing extendedPrice
            , flip (comparing actualQty)
            ]

-- nondeterministically accept/reject each supplier, 
-- remembering the choice and (if accepting) tallying 
-- the shipping cost
selectSupplier s = do
    mbPrev <- gets (M.lookup (supplierName s))
    case mbPrev of
        Just prev   -> return prev
        Nothing     -> do
            accept <- pure True <|> pure False
            modify (M.insert (supplierName s) accept)
            when accept (tell (Sum (shipping s)))
            return accept
