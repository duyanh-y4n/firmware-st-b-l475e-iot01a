# Training model 

## input/featrure
- MFCC: 
	- window time: 1s
	- wwidow increase: 0.5s
	- MFCC window: 40ms
	- MFCC stride: 20ms

## common hypermeter
- learning rate: 0.005

## layer

### 2d conv
2DC (filters, kernel size, layers)

### 1d conv
1DC (filters, kernel size, layers)

### Dense (FC/MLP)
1DC (filters, kernel size, layers)

### Max pooling
- for all: size = (2,2)
- stride = 1

## performance

### 1DConv

| architect                     | batch | epoch num | training time/ epoch | acc |
| ----------------------------- | ----- | --------- | -------------------- | --- |
| 1DC(16,3,1) 1DC(32,3,1) (128) |       |           |                      |     |
| 1DC(16,3,1) 1DC(32,3,1) (64)  | 32    | 200       | 8                    | 72  |
| 1DC(16,3,3) 1DC(32,3,3)       | 32    | 200       | 8                    |     |
| 1DC(16,3,3) 1DC(32,3,3) (128) |       |           |                      |     |
| 1DC(16,3,3) 1DC(32,3,3) (64)  |       |           |                      |     |
| 1DC(8,3,1) 1DC(16,3,1) (64)   |       |           |                      |     |
| 1DC(8,3,1) 1DC(16,3,3) (164)  |       |           |                      |     |
| 1DC(16,3,1) 1DC(32,3,1)       | 512   | 200       | 4                    | 77  |
| 1DC(8,3,3) 1DC(16,3,3)        |       |           |                      |     |
| 1DC(8,3,3) 1DC(16,3,3) (128)  |       |           |                      |     |

### 2DConv

| architect                     | batch | epoch num | training time/ epoch | acc |
| ----------------------------- | ----- | --------- | -------------------- | --- |
| 2DC(16,3,1) 1DC(32,3,1) (128) |       |           |                      |     |
| 2DC(16,3,1) 1DC(32,3,1) (64)  |       |           |                      |     |
| 2DC(16,3,3) 1DC(32,3,3)       |       |           |                      |     |
| 2DC(16,3,3) 1DC(32,3,3) (128) |       |           |                      |     |
| 2DC(16,3,3) 1DC(32,3,3) (64)  |       |           |                      |     |
| 2DC(8,3,1) 1DC(16,3,1) (64)   |       |           |                      |     |
| 2DC(8,3,1) 1DC(16,3,3) (164)  |       |           |                      |     |
| 2DC(8,3,3) 1DC(16,3,3)        |       |           |                      |     |
| 2DC(8,3,3) 1DC(16,3,3) (128)  |       |           |                      |     |


### mix 1DConv and 2Dconv