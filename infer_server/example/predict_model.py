# importing the libraries
import torch
import torchvision
from torchvision import datasets, transforms
from torch import nn, optim
from model import Net



transform = transforms.Compose([transforms.ToTensor(),
                                transforms.Normalize((0.5,), (0.5,)),
                                ])
trainset = datasets.FashionMNIST('./train_data', download=True, train=True, transform=transform)
testset = datasets.FashionMNIST('./test_data', download=True, train=False, transform=transform)

trainloader = torch.utils.data.DataLoader(trainset, batch_size=64, shuffle=True)
testloader = torch.utils.data.DataLoader(testset, batch_size=64, shuffle=True)


# defining the model architecture

# defining the model
model = Net()
# defining the optimizer
optimizer = optim.Adam(model.parameters(), lr=0.01)
# defining the loss function
criterion = nn.CrossEntropyLoss()


print(model)

epoch = 10

for i in range(epoch):
    running_loss = 0
    for images, labels in trainloader:

        if torch.cuda.is_available():
            images = images.cuda()
            labels = labels.cuda()

        # Training pass
        optimizer.zero_grad()

        output = model({'img': images})["result"]
        loss = criterion(output, labels)

        #This is where the model learns by backpropagating
        loss.backward()

        #And optimizes its weights here
        optimizer.step()

        running_loss += loss.item()
    else:
        print("Epoch {} - Training loss: {}".format(i+1, running_loss/len(trainloader)))

#model.save("save.pt")

#for images, labels in testloader:
#    img = images[0].view(1, 1, 28, 28)
#    traced_cell = torch.jit.trace(model, img)
#    print(traced_cell.graph)
#    print(traced_cell.code)
#    traced_cell.save("model.pt")
#    break



prediction = []
label = []

correct_count, all_count = 0, 0
for images, labels in testloader:
    for i in range(len(labels)):
        if torch.cuda.is_available():
            images = images.cuda()
            labels = labels.cuda()
        img = images[i].view(1, 1, 28, 28)
        with torch.no_grad():
            logps = model({'img': img})["result"]
        ps = torch.exp(logps)
        probab = list(ps.cpu()[0])
        pred_label = probab.index(max(probab))
        true_label = labels.cpu()[i]
        if(true_label == pred_label):
            correct_count += 1
        all_count += 1
        prediction.append(pred_label)
        label.append(true_label)
        pass

print("Number Of Images Tested =", all_count)
print("\nModel Accuracy =", (correct_count/all_count))

scripted_cell = torch.jit.script(model)
print(scripted_cell.code)
scripted_cell.save("demo.pt")
torch.save(model, "mode_save.pt")

