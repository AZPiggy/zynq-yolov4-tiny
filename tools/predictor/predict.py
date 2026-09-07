from PIL import Image
from yolo import YOLO

if __name__ == "__main__":
    yolo = YOLO()
    mode=0                    #0为图片预处理,生成可以直接作为加速器输入的img.bin文件,1为根据加速器的输出进行解码、显示
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


    
