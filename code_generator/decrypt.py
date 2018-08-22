import csv

file_encrypted='encrypted.csv'
file_passwords='passwords.csv'
file_decrypted='decrypted.csv'

with open(file_encrypted, newline='') as csv1, \
     open(file_passwords, newline='') as csv2, \
     open(file_decrypted, 'w', newline='') as csv3:
    csvencrypted = csv.reader(csv1, delimiter=',')
    csvpasswords = list(csv.reader(csv2, delimiter=','))
    csvdecrypted = csv.writer(csv3, delimiter=',')
    for dimension_id, encrypted in csvencrypted:
        for password_id, password in csvpasswords:
            decrypted = format(int(str(encrypted), 16) ^ int(str(password+password+password+password), 16), '0>16X')
            csvdecrypted.writerow([dimension_id+'-'+password_id, encrypted, password, decrypted])
