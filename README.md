# 🌐 Webserv - HTTP/1.1 Server

Un servidor web completo implementado en C++98 como proyecto de 42 School.

## 📋 Tabla de Contenidos

- [Compilación](#compilación)
- [Ejecución](#ejecución)
- [Uso Básico](#uso-básico)
- [Características](#características)
- [Pruebas](#pruebas)
- [Configuración](#configuración)
- [Troubleshooting](#troubleshooting)

## 🔨 Compilación

```bash
# Clonar el repositorio (si es necesario)
git clone [tu-repositorio]
cd webserv

# Compilar el proyecto
make

# Para recompilar desde cero
make re

# Para limpiar archivos objeto
make clean

# Para limpiar todo
make fclean
```

## 🚀 Ejecución

### Inicio Básico

```bash
# Ejecutar con configuración por defecto
./webserv configs/default.conf

# Ejecutar con otra configuración
./webserv configs/test.conf
```

Deberías ver algo como:
```
✓ Listening on 0.0.0.0:8080
✓ Listening on 0.0.0.0:8081
✓ Listening on 127.0.0.1:8082

🚀 Webserv started successfully!
```

## 🌟 Uso Básico

### Acceder desde el Navegador

Una vez que el servidor esté ejecutándose, abre tu navegador web y visita:

| URL | Descripción |
|-----|-------------|
| **http://localhost:8080** | Página principal del servidor |
| **http://localhost:8080/index.html** | Página de inicio (misma que arriba) |
| **http://localhost:8080/notfound** | Prueba página 404 personalizada |
| **http://localhost:8081** | Servidor secundario (puerto 8081) |
| **http://localhost:8082** | Servidor de desarrollo (puerto 8082) |

### Usando cURL (Terminal)

```bash
# GET Request - Obtener página principal
curl http://localhost:8080/

# GET Request - Con headers detallados
curl -v http://localhost:8080/

# POST Request - Enviar datos
curl -X POST -d "name=test&value=123" http://localhost:8080/upload

# POST Request - Subir archivo
curl -X POST -F "file=@/path/to/file.txt" http://localhost:8080/upload

# DELETE Request - Eliminar archivo
curl -X DELETE http://localhost:8080/upload/file.txt

# Ver headers de respuesta
curl -I http://localhost:8080/
```

## ✨ Características

### 1. **Métodos HTTP Soportados**
- ✅ **GET**: Obtener recursos
- ✅ **POST**: Enviar datos/subir archivos
- ✅ **DELETE**: Eliminar recursos

### 2. **Funcionalidades Principales**

#### 📁 **Servidor de Archivos Estáticos**
```bash
# Acceder a archivos HTML, CSS, JS, imágenes, etc.
http://localhost:8080/index.html
http://localhost:8080/styles.css
http://localhost:8080/images/logo.png
```

#### 📤 **Subida de Archivos**
```bash
# Subir un archivo usando curl
curl -X POST -F "upload=@foto.jpg" http://localhost:8080/upload

# Subir usando formulario HTML
# Crear un archivo test_upload.html:
```
```html
<form action="http://localhost:8080/upload" method="post" enctype="multipart/form-data">
    <input type="file" name="file">
    <input type="submit" value="Upload">
</form>
```

#### 📂 **Listado de Directorios (Autoindex)**
```bash
# Si está habilitado en la configuración
http://localhost:8080/public/
http://localhost:8082/  # Dev server tiene autoindex activado
```

#### 🔧 **CGI Support**

**Python CGI:**
1. Crear archivo `www/cgi-bin/hello.py`:
```python
#!/usr/bin/env python3
print("Content-Type: text/html\n")
print("<html><body>")
print("<h1>Hello from Python CGI!</h1>")
print("<p>Time: " + str(__import__('datetime').datetime.now()) + "</p>")
print("</body></html>")
```

2. Hacer ejecutable:
```bash
chmod +x www/cgi-bin/hello.py
```

3. Acceder:
```bash
http://localhost:8080/cgi-bin/hello.py
```

**PHP CGI:**
1. Crear archivo `www/test.php`:
```php
<?php
echo "<h1>Hello from PHP!</h1>";
echo "<p>Server time: " . date('Y-m-d H:i:s') . "</p>";
phpinfo();
?>
```

2. Acceder:
```bash
http://localhost:8080/test.php
```

## 🧪 Pruebas

### Test Básicos

```bash
# 1. Test servidor funcionando
curl -I http://localhost:8080/

# 2. Test 404
curl http://localhost:8080/esta-pagina-no-existe

# 3. Test método no permitido
curl -X PUT http://localhost:8080/

# 4. Test subida de archivo
echo "test content" > test.txt
curl -X POST -F "file=@test.txt" http://localhost:8080/upload

# 5. Test eliminación
curl -X DELETE http://localhost:8080/upload/test.txt

# 6. Test CGI Python
curl http://localhost:8080/cgi-bin/test.py

# 7. Test redirección
curl -L http://localhost:8080/old-page
```

### Stress Test

```bash
# Instalar Apache Bench si no lo tienes
# Ubuntu/Debian: sudo apt-get install apache2-utils
# MacOS: ya viene instalado

# Test de concurrencia - 1000 requests, 10 concurrentes
ab -n 1000 -c 10 http://localhost:8080/

# Test con POST
ab -n 100 -c 5 -p post_data.txt -T application/x-www-form-urlencoded http://localhost:8080/upload
```

### Test con Telnet (HTTP Raw)

```bash
# Conectar con telnet
telnet localhost 8080

# Escribir (pegar todo junto y presionar Enter dos veces):
GET / HTTP/1.1
Host: localhost
Connection: close

```

### Test con Navegador

1. **Abrir Developer Tools** (F12 en la mayoría de navegadores)
2. **Ir a la pestaña Network**
3. **Visitar** `http://localhost:8080`
4. **Verificar**:
   - Status codes correctos (200, 404, etc.)
   - Headers de respuesta
   - Tiempos de carga

## ⚙️ Configuración

### Estructura del Archivo de Configuración

```nginx
server {
    # Configuración de red
    listen 8080                     # Puerto de escucha
    host 0.0.0.0                   # IP de binding
    server_name localhost          # Nombre del servidor

    # Límites
    client_max_body_size 10485760  # Tamaño máximo del body (bytes)

    # Páginas de error personalizadas
    error_page 404 /error/404.html
    error_page 500 /error/500.html

    # Locations (rutas)
    location / {
        root www                    # Directorio raíz
        index index.html           # Archivo índice
        allow GET POST             # Métodos permitidos
        autoindex off              # Listado de directorio
    }

    location .php {
        cgi_path /usr/bin/php-cgi  # Path al intérprete
        cgi_extension .php         # Extensión a procesar
    }
}
```

### Crear tu Propia Configuración

1. Copiar configuración de ejemplo:
```bash
cp configs/default.conf configs/mi_config.conf
```

2. Editar según necesidades:
```bash
nano configs/mi_config.conf
```

3. Ejecutar:
```bash
./webserv configs/mi_config.conf
```

## 🐛 Troubleshooting

### Problemas Comunes

| Problema | Solución |
|----------|----------|
| **"Address already in use"** | Otro proceso usa el puerto. Cambiar puerto o matar proceso: `lsof -i :8080` y `kill -9 [PID]` |
| **"Permission denied" al bind** | Intentando usar puerto < 1024. Usar puerto > 1024 o ejecutar con sudo |
| **404 en todas las páginas** | Verificar que el directorio `www/` existe y contiene archivos |
| **CGI no funciona** | Verificar que el script es ejecutable: `chmod +x script.py` |
| **"Failed to fork process"** | Sistema sin recursos. Cerrar otros programas |
| **Conexión rechazada** | Firewall bloqueando. Verificar firewall o usar localhost |

### Debug Mode

Para ver más información de debug:

```bash
# Ver logs en tiempo real
./webserv configs/default.conf

# En otra terminal, hacer requests y ver output en la primera
curl -v http://localhost:8080/
```

### Verificar Puertos en Uso

```bash
# Linux
netstat -tlnp | grep 8080

# MacOS
lsof -i :8080

# Ver todos los puertos en uso
ss -tulnp
```

## 📝 Archivos de Estructura

```
webserv/
├── webserv              # Ejecutable (después de compilar)
├── configs/
│   ├── default.conf     # Configuración principal
│   └── test.conf        # Configuraciones de prueba
├── www/
│   ├── index.html       # Página principal
│   ├── error/
│   │   ├── 404.html    # Página error 404
│   │   └── 500.html    # Página error 500
│   ├── uploads/        # Directorio para subidas
│   ├── cgi-bin/        # Scripts CGI
│   └── public/         # Archivos públicos
└── src/                # Código fuente
```

## 🎯 Ejemplos de Uso Real

### 1. Servidor de Archivos Personal
```bash
# Compartir archivos en red local
./webserv configs/default.conf
# Otros dispositivos pueden acceder a http://[tu-ip]:8080
```

### 2. API REST Simple
```bash
# GET datos
curl http://localhost:8081/api/users

# POST nuevo usuario
curl -X POST -H "Content-Type: application/json" \
     -d '{"name":"John","age":30}' \
     http://localhost:8081/api/users

# DELETE usuario
curl -X DELETE http://localhost:8081/api/users/123
```

### 3. Servidor de Desarrollo
```bash
# Con autoindex act
