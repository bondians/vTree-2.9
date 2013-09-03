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

bom =
    [ (1, pcb)
    , (1, regulator)
    , (1, inputCap)
    , (1, outputCap)
    , (3, gateLimResistor)
    , (3, gatePullDownResistor)
    , (1, redVtgMatchResistor)
    , (3, mosfet)
    ]

pcb = 
    [ Part 
        { supplier      = oshPark
        , partNo        = "standalone power section pcb"
        , minimumQty    = 3
        , increment     = 3
        , price         = 1.00
        }
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
    ]

inputCap = basicPart mouser "AVE107M16X16T-F"
    [ (1,   0.12)
    , (50,  0.11)
    , (100, 0.092)
    , (500, 0.075)
    ]

outputCap = basicPart mouser "AVE476M06C12T-F"
    [ (1,   0.1)
    , (50,  0.078)
    , (100, 0.075)
    , (500, 0.061)
    ]

gateLimResistor = basicPart mouser "ERJ-2GEJ151X"
    [ (1,    0.08)
    , (10,   0.012)
    , (100,  0.005)
    , (1000, 0.004)
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
    ]

gatePullDownResistor = basicPart mouser "ERJ-2GEJ104X"
    [ (1,    0.08)
    , (10,   0.012)
    , (100,  0.005)
    , (1000, 0.004)
    ]

mosfet = basicPart mouser "SI2302CDS-T1-E3"
    [ (1,    0.15)
    , (10,   0.14)
    , (100,  0.13)
    , (250,  0.125)
    , (500,  0.12)
    , (1000, 0.115)
    ]

---------------------------------------

data Supplier = Supplier
    { supplierName  :: String
    , shipping      :: Double -- TODO: [(Integer, Part)] -> Double
    } deriving (Eq, Ord, Read, Show)

mouser  = Supplier "Mouser" 4.99
oshPark = Supplier "OSH Park" 0
digikey = Supplier "Digikey" 5.47
newark  = Supplier "Newark" 8.50

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
