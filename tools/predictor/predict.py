from PIL import Image
from yolo import YOLO

if __name__ == "__main__":
    yolo = YOLO()
    mode=0                    #0 = preprocess an image into img.bin for the accelerator; 1 = decode and display the accelerator's output
    mode=int(input('Input mode(0:preprocess,1:detect):'))
    if mode==1:
        dir = input('Input directory:')
        img_name=input("Input image name:")
        try:
            image = Image.open(dir+"\\"+img_name)
        except:
            print('Open Error! Try again!')
        else:
            r_image = yolo.detect_image(image,dir)
            r_image.show()
    elif mode==0:
        image = input("Input image:")
        dir = input('Output directory:')
        try:
            image = Image.open(image)
        except:
            print('Open Error! Try again!')
        yolo.preprocess_image(image,dir)


    
