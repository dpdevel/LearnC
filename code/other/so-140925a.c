/*Scrivere una applicazione C/POSIX costituita da diversi processi interagenti.
L’applicazione legge dalla linea comando una serie di nomi di file ed avvia un processo per
ciascun file specificato. Ciascun processo dovrà leggere il proprio file una riga di testo alla
volta, e scrivere tale riga sullo standard output dell’applicazione, rispettando il seguente
ordinamento:
• Prima riga del file del primo processo
• Prima riga del file del secondo processo
..
.
• Prima riga del file dell’ultimo processo
• Seconda riga del file del primo processo
• Seconda riga del file del secondo processo
..
.
L’applicazione deve gestire correttamente il caso in cui i file non sono costituiti dallo stesso
numero di righe di testo (nel turno corrispondente il processo relativo non scriverà nulla in
standard output). Prestare attenzione agli aspetti della sincronizzazione tra i processi e della
efficienza dell’applicazione, evitando di introdurre ritardi artificiali per forzare l’ordinamento
delle righe scritte sullo standard output.*/



