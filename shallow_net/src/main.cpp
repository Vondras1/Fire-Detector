#include <Arduino.h>
#include "model_data.h"
#include <EloquentTinyML.h>

// Define the number of inputs and outputs for the model
#define NUMBER_OF_INPUTS  3
#define NUMBER_OF_OUTPUTS 1

// Define the size of the tensor arena (used for memory allocation during inference)
// For small models, 2–4 KB is usually enough
#define TENSOR_ARENA_SIZE (2 * 1024)

// Create an instance of the TfLite interpreter for the given model
Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

void setup() {
    Serial.begin(9600);
    delay(1000);

    // Initialize the model from the included model data array
    ml.begin(model_tflite);
    Serial.println("Model is ready!");
}

void loop() {
    // Generate random input values (x1, x2, x3) for demonstration
    float x1 = random(100) / 100.0;
    float x2 = random(100) / 100.0;
    float x3 = random(100) / 100.0;

    // Store input values in an array as expected by EloquentTinyML
    float input[NUMBER_OF_INPUTS] = { x1, x2, x3 };

    // Predict output
    float predicted = ml.predict(input); // pst: (x1*0.2 + x2*0.5 + x3*0.3) > 0.7

    // Print input and predicted value

    Serial.print("Input: ");
    Serial.print(x1); Serial.print(", ");
    Serial.print(x2); Serial.print(", ");
    Serial.print(x3);
    Serial.print("  =>  predicted: ");
    Serial.print(predicted);
    Serial.print("  =>  golden value: ");
    Serial.println(x1*0.2 + x2*0.5 + x3*0.3);

    delay(1000);
}