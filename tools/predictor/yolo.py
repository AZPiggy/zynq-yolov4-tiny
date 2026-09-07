import colorsys
import numpy as np
import torch
from PIL import ImageDraw, ImageFont
from utils.utils import (cvtColor, get_anchors, get_classes, preprocess_input,resize_image)
from utils.utils_bbox import DecodeBox

class YOLO(object):
    _defaults = {
        #--------------------------------------------------------------------------#
        "classes_path"      : 'model_data/coco_classes.txt',
        #---------------------------------------------------------------------#
        #   anchors_path points at the anchor definition file; normally left alone.
        #   anchors_mask selects which anchors belong to which head; normally left alone.
        #---------------------------------------------------------------------#
        "anchors_path"      : 'model_data/yolo_anchors.txt',
        "anchors_mask"      : [[3,4,5], [1,2,3]],
        #-------------------------------#
        #---------------------------------------------------------------------#
        #   Network input size; must be a multiple of 32.
        #---------------------------------------------------------------------#
        "input_shape"       : [416, 416],
        #---------------------------------------------------------------------#
        #   Only boxes scoring above this confidence are kept
        #---------------------------------------------------------------------#
        "confidence"        : 0.5,
        #---------------------------------------------------------------------#
        #   IoU threshold used by non-maximum suppression
        #---------------------------------------------------------------------#
        "nms_iou"           : 0.3,
        #---------------------------------------------------------------------#
        #   Controls whether letterbox_image is used for aspect-preserving resize;
        #   testing showed a plain resize works better here, so it is disabled
        #---------------------------------------------------------------------#
        "letterbox_image"   : False,
    }

    @classmethod
    def get_defaults(cls, n):
        if n in cls._defaults:
            return cls._defaults[n]
        else:
            return "Unrecognized attribute name '" + n + "'"

    #---------------------------------------------------#
    #   Initialise YOLO
    #---------------------------------------------------#
    def __init__(self, **kwargs):
        self.__dict__.update(self._defaults)
        for name, value in kwargs.items():
            setattr(self, name, value)
        #---------------------------------------------------#
        #   Count the classes and anchors
        #---------------------------------------------------#
        self.class_names, self.num_classes  = get_classes(self.classes_path)
        self.out_ch=(5+self.num_classes)*3
        self.anchors, self.num_anchors      = get_anchors(self.anchors_path)
        self.bbox_util                      = DecodeBox(self.anchors, self.num_classes, (self.input_shape[0], self.input_shape[1]), self.anchors_mask)
        #---------------------------------------------------#
        #   Assign a distinct colour per class
        #---------------------------------------------------#
        hsv_tuples = [(x / self.num_classes, 1., 1.) for x in range(self.num_classes)]
        self.colors = list(map(lambda x: colorsys.hsv_to_rgb(*x), hsv_tuples))
        self.colors = list(map(lambda x: (int(x[0] * 255), int(x[1] * 255), int(x[2] * 255)), self.colors))
    #Preprocess an image so it can be fed straight to the accelerator
    def preprocess_image(self,image,dir):
        image       = cvtColor(image)
        image_data  = resize_image(image, (self.input_shape[1],self.input_shape[0]), self.letterbox_image)
        image_data  = np.expand_dims(np.transpose(preprocess_input(np.array(image_data, dtype='float32')), (2, 0, 1)), 0)
        images = torch.from_numpy(image_data)
        import os
        if dir!=None:
            if not os.path.exists(dir):
                os.mkdir(dir)
            images.numpy().tofile(dir+"\\img.bin")
            image.save(dir+"\\img.png")
        return images
    #---------------------------------------------------#
    #   Run detection on an image
    #---------------------------------------------------#
    def detect_image(self, image,dir):
        image_shape = np.array(np.shape(image)[0:2])
        images=self.preprocess_image(image,dir=None)
        with torch.no_grad():
            #Use the tensors produced by the accelerator directly:
            out0=torch.from_numpy(np.fromfile(dir+"\\out0.bin",dtype=np.float32)).view(1,self.out_ch,13,13)
            out1=torch.from_numpy(np.fromfile(dir+"\\out1.bin",dtype=np.float32)).view(1,self.out_ch,26,26)
            outputs=[out0,out1]
            outputs = self.bbox_util.decode_box(outputs)
            #---------------------------------------------------------#
            #   Stack the predictions, then run non-maximum suppression
            #---------------------------------------------------------#
            results = self.bbox_util.non_max_suppression(torch.cat(outputs, 1), self.num_classes, self.input_shape, 
                        image_shape, self.letterbox_image, conf_thres = self.confidence, nms_thres = self.nms_iou)
                                                    
            if results[0] is None: 
                return image

            top_label   = np.array(results[0][:, 6], dtype = 'int32')
            top_conf    = results[0][:, 4] * results[0][:, 5]
            top_boxes   = results[0][:, :4]
        #---------------------------------------------------------#
        #   Set the font and box thickness
        #---------------------------------------------------------#
        font        = ImageFont.truetype(font='model_data/simhei.ttf', size=np.floor(3e-2 * image.size[1] + 0.5).astype('int32'))
        thickness   = int(max((image.size[0] + image.size[1]) // np.mean(self.input_shape), 1))
        
        #---------------------------------------------------------#
        #   Draw onto the image
        #---------------------------------------------------------#
        for i, c in list(enumerate(top_label)):
            predicted_class = self.class_names[int(c)]
            box             = top_boxes[i]
            score           = top_conf[i]

            top, left, bottom, right = box

            top     = max(0, np.floor(top).astype('int32'))
            left    = max(0, np.floor(left).astype('int32'))
            bottom  = min(image.size[1], np.floor(bottom).astype('int32'))
            right   = min(image.size[0], np.floor(right).astype('int32'))

            label = '{} {:.2f}'.format(predicted_class, score)
            draw = ImageDraw.Draw(image)
            label_size = draw.textsize(label, font)
            label = label.encode('utf-8')
            print(label, top, left, bottom, right)
            
            if top - label_size[1] >= 0:
                text_origin = np.array([left, top - label_size[1]])
            else:
                text_origin = np.array([left, top + 1])

            for i in range(thickness):
                draw.rectangle([left + i, top + i, right - i, bottom - i], outline=self.colors[c])
            draw.rectangle([tuple(text_origin), tuple(text_origin + label_size)], fill=self.colors[c])
            draw.text(text_origin, str(label,'UTF-8'), fill=(0, 0, 0), font=font)
            del draw

        return image

    

    
