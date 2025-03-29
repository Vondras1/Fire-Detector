import pandas as pd
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import argparse
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("--batch_size", default=32, type=int, help="Batch size.")
parser.add_argument("--epochs", default=10, type=int, help="Number of epochs.")
parser.add_argument("--seed", default=42, type=int, help="Random seed.")
parser.add_argument("--model", default="uppercase.pt", type=str, help="Output model path.")

class BatchGenerator:
    def __init__(self, data, batch_size):
        self._inputs = data[0]
        self._outputs = data[1]
        self._batch_size = batch_size

    def __len__(self):
        # Number of batches
        return (len(self._inputs) + self._batch_size - 1) // self._batch_size

    def __iter__(self):
        indices = np.arange(len(self._inputs))
        for start in range(0, len(indices), self._batch_size):
            batch_indices = indices[start : (start + self._batch_size)]
            yield self._inputs[batch_indices], self._outputs[batch_indices]

class PrepareDataset:
    def __init__(self, dataset):
        self._dataset = dataset
    
    def __len__(self):
        return len(self._dataset)
    
    def get_data(self):
        input_data = self._dataset[["x1", "x2", "x3"]].values
        label_data = self._dataset["labels"].values
        return (input_data, label_data)
    
class Model(tf.keras.Model):
    def __init__(self, args: argparse.Namespace) -> None:
        super().__init__()
        self.args = args

        self.model = keras.Sequential([
            layers.Input(shape=(3,)),
            layers.Dense(10, activation="relu"),
            layers.Dense(1, activation="sigmoid")
        ])
    
    def call(self, input):
        return self.model(input)

def main(args: argparse.Namespace) -> None:
    
    # Load the data.
    data = pd.read_csv("data.csv")

    # Shuffle the data
    data = data.sample(frac=1, random_state=42).reset_index(drop=True)

    # Prepare data - Split into batches
    n = len(data)
    train_data = data[:int(n*0.7)]
    dev_data   = data[int(n*0.7):int(n*0.85)]
    test_data  = data[int(n*0.85):]

    input_train, label_train = PrepareDataset(train_data).get_data()
    input_dev, label_dev = PrepareDataset(dev_data).get_data()
    input_test, label_test = PrepareDataset(test_data).get_data()

    # Model
    model = Model(args)
    # Compile
    model.compile(optimizer="adam",
                loss="binary_crossentropy",
                metrics=["accuracy"])

    # Train
    model.fit(input_train, label_train, validation_data=(input_dev, label_dev),
              batch_size=args.batch_size, epochs=args.epochs)

    # Evaluate
    test_loss, test_acc = model.evaluate(input_test, label_test)
    print(f"Test accuracy: {test_acc:.2f}")

    model.model.save("model.h5")

if __name__ == "__main__":
    main_args = parser.parse_args([] if "__file__" not in globals() else None)
    main(main_args)

