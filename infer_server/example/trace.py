
import torch
import torchvision
from torchvision import datasets, transforms
from torch import nn, optim
import json
from model import Net

transform = transforms.Compose([transforms.ToTensor(),
                                transforms.Normalize((0.5,), (0.5,)),
                                ])
trainset = datasets.FashionMNIST('./train_data', download=True, train=True, transform=transform)
testset = datasets.FashionMNIST('./test_data', download=True, train=False, transform=transform)

trainloader = torch.utils.data.DataLoader(trainset, batch_size=64, shuffle=True)
testloader = torch.utils.data.DataLoader(testset, batch_size=64, shuffle=True)

model = torch.load("mode_save.pt")
model.eval()

with open('dump.json', 'w+') as writer:
    for images, labels in testloader:
        print(images.shape)
        for i in range(len(labels)):
            img = images[i].view(1, 1, 28, 28)
            print(model({'img':img}))
            print(img.shape)
            newimg = img.reshape((28*28))
            data = {
                "shape": [1, 1, 28, 28],
                "data": img.reshape((28*28)).tolist(),
                "result": labels.cpu()[i].tolist()
            }
            writer.write(json.dumps(data))
            writer.write('\n')
            pass
        pass
    pass




