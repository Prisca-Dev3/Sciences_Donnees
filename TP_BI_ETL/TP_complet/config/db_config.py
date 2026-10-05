# Database Configuration
# Update these values with your actual database credentials

# Common defaults:
# user: 'root', password: '' (empty)
# user: 'root', password: 'root'
# user: 'student', password: 'password'

DB_USER = 'root' 
DB_PASSWORD = '' #Votre mot de passe ici
DB_HOST = 'localhost'

# Connection strings
# We use mysql+mysqlconnector driver
CONN_STR_VENTE_DB = f'mysql+mysqlconnector://{DB_USER}:{DB_PASSWORD}@{DB_HOST}/vente_db'
CONN_STR_VENTE_DW = f'mysql+mysqlconnector://{DB_USER}:{DB_PASSWORD}@{DB_HOST}/vente_DW'
