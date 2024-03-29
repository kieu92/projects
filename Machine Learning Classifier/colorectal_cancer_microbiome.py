# -*- coding: utf-8 -*-
"""Copy of Colorectal Cancer Microbiome

Automatically generated by Colaboratory.

Original file is located at
    https://colab.research.google.com/drive/1CNcfEKaUXROmQnI-eK4m-EofxUgJlPJB
"""

from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.svm import LinearSVC
import pandas as pd
import numpy as np 
from sklearn.naive_bayes import GaussianNB
from sklearn import metrics

###Load data in
otu_table = pd.read_csv("/content/otu.csv",index_col=0)

label_list = otu_table.columns.to_numpy()
otu_table_np = otu_table.to_numpy().transpose()

binary_labels = np.array([])

for j in label_list:
    if "Cancer" in j:
        binary_labels = np.append(binary_labels,1)

    elif "label" in j:
        pass

    else:
        binary_labels = np.append(binary_labels,0)

total_samples = otu_table_np
total_labels = binary_labels

###Split Data
training_data, testing_data, training_labels, testing_labels = train_test_split(total_samples, total_labels, test_size=0.2)

###Implement different models and assess their effectiveness
model = LinearSVC()
model.fit(training_data, training_labels)

y_pred = model.predict(testing_data)

accuracy = metrics.accuracy_score(testing_labels, y_pred) * 100

print(accuracy)