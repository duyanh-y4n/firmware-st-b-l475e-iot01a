import os
import serial.tools.list_ports
import sys
from PIL import Image
import serial
import time
# from struct import *

#####################################################
# pic format config
#####################################################
image_config_dict = {}
pic_size160x120 = 38400
pic_size320x240 = 153600
pic_size480x272 = 261120
pic_size640x480 = 614400

pic_sizeArray = [pic_size160x120, pic_size320x240, pic_size480x272, pic_size640x480]

# @TODO replace with qqvga qvga ... vga
flag160x120 = "160x120"
flag320x240 = "320x240"
flag480x272 = "480x272"
flag640x480 = "640x480"

resolutions = [flag160x120, flag320x240, flag480x272, flag640x480]


def send_config(ser, config, config_name):
    print("stm32 asking for " + config_name)
    print("sending " + config_name + ":" + str(config))
    ser.write(config.to_bytes(1, "little"))


#####################################################
# connect to serial port
#####################################################

def init_uart():
    uart_baudrate = 115200
    uart_bytesize = 8
    uart_parity = 'N'
    if os.name == 'posix':
        ser = serial.Serial('/dev/ttyUSB0')
        ser.bytesize = uart_bytesize
        ser.parity = uart_parity
        ser.baudrate = uart_baudrate
    elif os.name == 'nt':
        print("choose device index:")
        comlist = serial.tools.list_ports.comports()
        for i, elem in enumerate(comlist):
            print(str(i) + ":" + elem.device)
            sys.stdout.flush()
        idx = int(input())
        ser = serial.Serial(comlist[idx].device, uart_baudrate)
    if ser:
        print(
            "open " + ser.name +\
            "\nbaud: " + str(ser.baudrate) +\
            "\ndata format:" + str(ser.bytesize) +\
            str(ser.parity) + str( ser.stopbits))
        return ser


def read_camera_parameters():
    print("Enter number of frame, you want to convert: ")
    num_pics = int(input())

    print("Please choose a resolution : ")
    print(" 0: " + flag160x120 + "\n 1: " + flag320x240 + "\n 2: " + flag480x272 + "\n 3: " + flag640x480)
    resolution = int(input())

    print("Save image to extern Ram?(0 = no, 1 = yes)")
    bufferDestination = int(input())

    pic_size = pic_sizeArray[resolution]
    flag_size = resolutions[resolution]

    image_config_dict['num_pics'] = num_pics
    image_config_dict['pic_size'] = pic_size
    image_config_dict['flag_size'] = flag_size
    image_config_dict['resolution'] = resolution
    image_config_dict['destination'] = bufferDestination
    return image_config_dict
    # print(picsizeArray)


def send_camera_paramerter(ser, config_dict):
    ser.read()
    send_config(ser, config_dict['resolution'], "Resolution")
    ser.read()
    send_config(ser, config_dict['num_pics'], "Number of Images")
    ser.read()
    send_config(ser, config_dict['destination'], "Buffer Destination to save images")


def init_usb():
    """docstring for init_usb"""
    connected = False
    uart_baudrate = 115200
    if os.name == 'posix':
        while not connected:
            try:
                usb_ser = serial.Serial('/dev/ttyACM1')
            except serial.serialutil.SerialException:
                print("open failed retrying.....")
                time.sleep(1)
            else:
                print("connected: to usb")
                connected = True
                return usb_ser
    elif os.name == 'nt':
        while not connected:
            print("choose device index:")
            comlist = serial.tools.list_ports.comports()
            for i, elem in enumerate(comlist):
                print(str(i) + ":" + elem.device)
                sys.stdout.flush()
            idx = int(input())
            try:
                usb_ser = serial.Serial(comlist[idx].device, uart_baudrate)
            except serial.serialutil.SerialException:
                print("open failed retrying.....")
                time.sleep(1)
            else:
                print("connected: to usb")
                connected = True
                return usb_ser


def read_images(ser_connection, config_dict):
    raw_images = []
    data = []
    num_pics = config_dict['num_pics']
    pic_size = config_dict['pic_size']
    for i in range(0, num_pics):
        #####################################################
        # get data from uart-usb
        #####################################################
        print("reading data...")
        time1 = time.time()
        # usb
        data = ser_connection.read(pic_size)
        # uart
        # data = ser.read(pic_size)
        raw_images.append(data)
        time2 = time.time()
        print('{:s} function took {:.3f} ms'.format("usb", (time2 - time1) * 1000.0))
        print('\n' + 'Total size: ' + str(len(data)) + ' bytes')
        return raw_images


def write_images_to_files(raw_images):
    raw_img_postfix = ".raw"
    """docstring for write_images_to_files"""
    for idx, data in enumerate(raw_images):
        file_name = "image"
        file_name = file_name + str(idx) + raw_img_postfix
        raw_file = open(file_name, "wb")
        raw_file.write(data)
        raw_file.close()


def convert_images(config_dict):
    command = "convert"
    raw_img_postfix = ".raw"
    bmp_img_postfix = ".bmp"
    filename = "image"
    outputName = "image_out"
    num_pics = config_dict['num_pics']
    flag_size = config_dict['flag_size']
    flags = ["-size " + flag_size, "-sampling-factor 4:2:2", "-depth 8"]
    for i in range(0, num_pics):
        inputfile = "yuv:" + filename + str(i + 0) + raw_img_postfix
        outputFile = outputName + str(i + 0) + bmp_img_postfix

        os.system(command + " " + ' '.join(flags) + " " + inputfile + " " + outputFile)
        os.system("rm" + " " + filename + str(i + 0) + raw_img_postfix)
        print(inputfile + " => " + outputFile + " \n")
        #####################################################
        # Image open
        #####################################################
        img = Image.open(outputFile)
        img.show()


def main():
    """docstring for main"""
    ser = init_uart()
    image_param = read_camera_parameters()
    send_camera_paramerter(ser, image_param)
    usb_ser = init_usb()
    images = read_images(usb_ser, image_param)
    write_images_to_files(images)
    convert_images(image_param)
    # show_image()
    usb_ser.close()
    ser.close()


if __name__ == '__main__':
    main()
