import tensorflow as tf

# Načti model
model = tf.keras.models.load_model("model.h5")

# Konvertuj na TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]  # pro kvantizaci
tflite_model = converter.convert()

# Ulož
with open("model.tflite", "wb") as f:
    f.write(tflite_model)