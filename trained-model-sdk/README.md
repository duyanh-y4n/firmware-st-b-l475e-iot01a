# Trained model sdk

Here are the trained model for deployment.

## how to update model

- choose model
- unzip file to source folder
- compile test firmware and run command AT+RUNIMPULSE or AT+RUNIMPULSECONT

## model description

### trained-model-sdk/speech_command_interface-v1.zip

+ cmd: go, stop
+ feature: mfcc
+ model architecture

```python
# model architecture
model = Sequential()
model.add(Reshape((int(input_length / 13), 13), input_shape=(input_length, )))
model.add(Conv1D(8, kernel_size=3, activation='relu', padding='same'))
model.add(MaxPooling1D(pool_size=2, strides=2, padding='same'))
model.add(Dropout(0.25))
model.add(Conv1D(16, kernel_size=3, activation='relu', padding='same'))
model.add(MaxPooling1D(pool_size=2, strides=2, padding='same'))
model.add(Dropout(0.25))
model.add(Flatten())
model.add(Dense(classes, activation='softmax', name='y_pred'))

# this controls the learning rate
opt = Adam(lr=0.005, beta_1=0.9, beta_2=0.999)
# this controls the batch size, or you can manipulate the tf.data.Dataset objects yourself
BATCH_SIZE = 32
train_dataset, validation_dataset = ei_tensorflow.training.set_batch_size(BATCH_SIZE, train_dataset, validation_dataset)
callbacks.append(BatchLoggerCallback(BATCH_SIZE, train_sample_count))
```

+ performance
[[speech-command-interface-v1.png]]

### trained-model-sdk/speech_command_interface-v3.zip

+ 16 cmd:  unknown  backward  follow  forward  four  go  left  no  off  on  one  right  stop  three  two  visual  yes
+ feature: mfcc
+ model architecture
```python
# model architecture
model = Sequential()
model.add(Reshape((int(input_length / 13), 13), input_shape=(input_length, )))
model.add(Conv1D(16, kernel_size=3, activation='relu', padding='same'))
model.add(MaxPooling1D(pool_size=2, strides=2, padding='same'))
model.add(Dropout(0.25))
model.add(Conv1D(32, kernel_size=3, activation='relu', padding='same'))
model.add(MaxPooling1D(pool_size=2, strides=2, padding='same'))
model.add(Dropout(0.25))
model.add(Flatten())
model.add(Dense(classes, activation='softmax', name='y_pred'))

# this controls the learning rate
opt = Adam(lr=0.005, beta_1=0.9, beta_2=0.999)
# this controls the batch size, or you can manipulate the tf.data.Dataset objects yourself
BATCH_SIZE = 512
train_dataset, validation_dataset = ei_tensorflow.training.set_batch_size(BATCH_SIZE, train_dataset, validation_dataset)
callbacks.append(BatchLoggerCallback(BATCH_SIZE, train_sample_count))
```

+ performance
[SCIv3](SCIv3.pdf)
