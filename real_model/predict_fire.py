import pandas as pd
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import argparse
import numpy as np

parser = argparse.ArgumentParser()
parser.add_argument("--batch_size", default=32, type=int, help="Batch size.")
parser.add_argument("--epochs", default=30, type=int, help="Number of epochs.")
parser.add_argument("--seed", default=42, type=int, help="Random seed.")
parser.add_argument("--learning_rate", default=0.001, type=float, help="Initial learning rate.")  # nový argument

class PrepareDataset:
    def __init__(self, dataset):
        self._dataset = dataset
    
    def __len__(self):
        return len(self._dataset)
    
    def get_data(self):
        input_data = self._dataset[["smoke", "flame", "gas"]].values
        label_data = self._dataset["label"].values
        return (input_data, label_data)
    
class Model(tf.keras.Model):
    def __init__(self, args: argparse.Namespace) -> None:
        super().__init__()
        self.args = args

        self.model = keras.Sequential([
            layers.Input(shape=(3,)),
            layers.Dense(18, activation="relu"),
            layers.Dense(1, activation="sigmoid")
        ])
    
    def call(self, input): #forward
        return self.model(input)
    
class PrintLearningRate(keras.callbacks.Callback):
    def on_epoch_end(self, epoch, logs=None):
        opt = self.model.optimizer
        lr = opt.learning_rate
        if isinstance(lr, tf.keras.optimizers.schedules.LearningRateSchedule):
            lr = lr(opt.iterations)
        lr = tf.keras.backend.get_value(lr)
        print(f"\nEpoch {epoch+1:02d}: learning rate = {lr:.8f}")

def main(args: argparse.Namespace) -> None:
    # Set seeds
    np.random.seed(args.seed)
    tf.random.set_seed(args.seed)
    
    # Load the data.
    data = pd.read_csv("all_data.csv") #all_data.csv => 1.tatka_taborak: TABOR4, TABOR6, TABOR7, 2.lhota: TABOR4, TABOR5, TABOR6, TABOR7

    # Shuffle the data
    data = data.sample(frac=1, random_state=args.seed).reset_index(drop=True)

    # Prepare data - Split into train/dev/test datasets
    n = len(data)
    train_data = data[:int(n*0.75)]
    dev_data   = data[int(n*0.75):int(n*0.80)]
    test_data  = data[int(n*0.75):]

    input_train, label_train = PrepareDataset(train_data).get_data()
    input_dev, label_dev = PrepareDataset(dev_data).get_data()
    input_test, label_test = PrepareDataset(test_data).get_data()

    # Model
    model = Model(args)

    # Number of steps per epoch
    steps_per_epoch = (len(train_data) // args.batch_size)
    # Total number of steps
    total_steps     = steps_per_epoch * args.epochs

    # cosine-decay, args.learning_rate → 0
    lr_schedule = tf.keras.optimizers.schedules.CosineDecay(
        initial_learning_rate=args.learning_rate,
        decay_steps=total_steps,
        alpha=0.0 
    )

    # Compile
    optimizer = tf.keras.optimizers.AdamW(learning_rate=lr_schedule)
    model.compile(optimizer=optimizer,
                loss="binary_crossentropy",
                metrics=["accuracy"])

    # Train
    print_lr = PrintLearningRate()
    model.fit(input_train, label_train, validation_data=(input_dev, label_dev),
              batch_size=args.batch_size, epochs=args.epochs, callbacks=[])

    # Evaluate
    test_loss, test_acc = model.evaluate(input_test, label_test)
    print(f"Test accuracy: {test_acc:.2f}")

    model.model.save("model_real5.h5")

if __name__ == "__main__":
    main_args = parser.parse_args([] if "__file__" not in globals() else None)
    main(main_args)

