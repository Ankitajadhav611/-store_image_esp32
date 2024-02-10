import serial
import time
import os

ser = serial.Serial('COM5', 115200) 
folder_path = 'test_image_folder'
files = os.listdir(folder_path)
counter = 2 #to make sure the connection is maintained for long time
while (counter !=0) :
    image_files = [f for f in files if f.lower().endswith(('.png', '.jpg'))]
    for image_file in image_files:
        image_path = os.path.join(folder_path, image_file)
        with open(image_path, 'rb') as f:
            image_data = f.read()
            length = len(image_data)
            print(length)
            ser.write(image_data)
        time.sleep(20)
    print("send count ",counter)
    counter -=1
        






