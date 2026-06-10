---
date: 2026-05-28
type: reference
tags: [lab, dimensions, geometry, room]
---

# Lab Space — Dimensions

User-provided rectangular lab envelope dimensions. The height, width, and length
are interior dimensions.

## Dimensions

| Quantity | Value |
|----------|-------|
| Height | 3.5 m |
| Interior x / width | 8.25 m |
| Interior y / length | 7.4 m |
| Wall thickness | 5 in |
| Wall material | Concrete |
| Opening length in y | 4.3 m |
| Opening height in z | 2.17 m |
| Opening start from interior +y edge | 0.9 m |

## Fridge Placement

The fridge center is positioned relative to the lab interior walls.

| Quantity | Value |
|----------|-------|
| Distance from right interior wall (+x) | 160 cm |
| Distance from bottom interior wall (-y) | 480 cm |
| Center x in lab interior frame | 2.525 m |
| Center y in lab interior frame | 1.10 m |
| Center z in lab interior frame | 1.7606 m |

## OVC Placement

| Quantity | Value |
|----------|-------|
| OVC top / bride bottom height above lab floor | 2.1 m |
| OVC can height | 103 cm |
| OVC inner diameter | 37.1 cm |
| OVC outer circumference | 120.5 cm |
| Calculated OVC wall thickness | 6.28 mm |
| OVC material | Stainless steel |
| OVC top ring outer diameter | 41.6 cm |
| OVC top ring height | 2.7 cm |

## Unit Conversions

| Quantity | Metric value |
|----------|--------------|
| Wall thickness | 0.127 m |
| Wall thickness | 127 mm |

## Notes

- The lab is described as a rectangular box.
- Height, width, and length are interior dimensions.
- The lab coordinate frame uses the room interior centerline for x/y, with
  `+x` toward the right wall and `-y` toward the bottom wall.
- The `+x` wall has a rectangular opening spanning `4.3 m` in `y` and
  `2.17 m` in `z`, starting `0.9 m` from the interior `+y` edge and running
  from floor level (`z = 0`) to `z = 2.17 m`.
- The fridge center z places the OVC top and bride bottom at 2.1 m above the
  lab floor when `/QR/geom/globalOffset` is `0 0 0 m`.
