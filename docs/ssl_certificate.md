## Protocolo SSL/TLS

**Autor: Nelson Spode**


O processo de comunicação no protocolo TLS/SSL consiste em duas partes. A primeira parte é o protocolo de handshake. O objetivo deste protocolo de handshake é identificar a identidade da outra parte e estabelecer um canal de comunicação seguro. Após um handshake, ambas as partes negociarão o próximo conjunto de criptografia e a chave de sessão. A segunda parte é o protocolo de registro. O registro é muito similar a outros protocolos de transmissão de dados. Ele carrega tipo de conteúdo, versão, comprimento, carga útil, etc, e a diferença é que as informações carregadas por este protocolo são criptografadas.

A figura a seguir descreve o processo do protocolo de handshake TLS/SSL. Do "hello" do cliente até "finished" do broker. Se você estiver interessado nisso, pode visualizar material mais detalhado. Mesmo que não saiba este processo, você também pode ativar esta função em [EMQX](https://www.emqx.com/en/products/emqx).

![what-is-ssl](https://assets.emqx.com/images/29b8bd83af006c104add0635a11682bb.gif?imageMogr2/thumbnail/1520x)


## Preparação do certificado SSL/TLS

Um certificado digital serve como um componente crucial para garantir a segurança e a autenticidade da comunicação e transações online. Seu objetivo principal é fornecer o seguinte:

1. **Autenticação**: Certificados verificam a identidade das partes envolvidas em uma comunicação. Frequentemente são usados para confirmar a identidade de um site para seus visitantes ou para verificar a identidade de um usuário ou dispositivo para um servidor. Esta autenticação ajuda a prevenir acesso não autorizado e garante que os usuários estejam interagindo com entidades legítimas.
    
2. **Comunicação Segura**: Certificados são usados na criptografia de transmissão de dados entre partes. Quando um certificado é empregado, permite conexões seguras e criptografadas. Essa criptografia garante que os dados trocados entre as partes não possam ser facilmente interceptados e lidos por atores maliciosos.
    
3. **Integridade de Dados**: Certificados também ajudam a manter a integridade dos dados. Ao usar assinaturas digitais e algoritmos de hashing, garantem que os dados recebidos não foram alterados durante a transmissão.
    
4. **Confiança e Segurança**: Certificados digitais são emitidos e verificados por autoridades de certificação confiáveis (CAs). Essas CAs desempenham um papel vital no estabelecimento de confiança na internet. Quando você vê o ícone de cadeado no seu navegador web ou "https://" em uma URL, isso indica uma conexão segura apoiada por um certificado válido. Isso infunde confiança nos usuários de que seus dados estão seguros.

Uma Autoridade de Certificação (CA) é uma entidade confiável responsável por emitir certificados digitais e verificar a identidade de indivíduos (**"Cartório"**), dispositivos ou serviços no contexto de uma infraestrutura de chave pública (PKI). As CAs desempenham um papel central no estabelecimento de confiança e segurança nas comunicações online. Aqui está uma descrição da Autoridade de Certificação:

### Comprar certificado

O certificado pode ser emitido por uma CA confiável, como por exemplo, Let's Encrypt, DigiCert, GlobalSign, etc. Essas CAs são amplamente reconhecidas e confiáveis por navegadores e sistemas operacionais. 

### O certificado auto-assinado

Muitas vezes Certificados auto-assinados podem ser usandos em dispositivos IoT para estabelecer conexões seguras. Eles são criados e assinados pela própria entidade que os utiliza, em vez de serem emitidos por uma CA confiável. Embora os certificados auto-assinados possam fornecer criptografia, eles não oferecem o mesmo nível de confiança que os certificados emitidos por CAs confiáveis, pois não são verificados por terceiros confiáveis.

#### Processo de criação de certificado auto-assinado

> Assumimos que o sistema tem OpenSSL instalado.

#### Passo 1 - Gerar a chave privada e o certificado raiz (CA Certificate)

Primeiramente, precisamos de uma chave privada para assinar o certificado raiz. Você pode executar o seguinte comando para gerar esta chave privada:

**Sempre gere a chave privada primeiro**

```
openssl genrsa -out ca.key 2048
```

Este comando gerará uma chave com comprimento de 2048 e será armazenada em `ca.key`. Se você tiver esta chave, pode usá-la para gerar o certificado raiz.

- Lembre que podemos criar uma cadeia de certificados. o certificado raiz é o emissor principal.

Agora, usando a chave privada, podemos gerar o certificado raiz:
```
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -out ca.pem
```

- Aqui estamos usando tipo de certificado x509;
- sha256 para fazer hash da assinatura do certificado.
- Certificado válido por 10 anos;
- Saída de um certificado chamado ca.pem (qualquer nome pode ser usado para chave e certificado); 

Para ver informações do certificado CA, podemos usar o seguinte comando:

```
openssl x509 -in ca.pem -noout -text
```

O certificado raiz é o ponto de partida de uma cadeia inteira de confiança. Se o emissor de cada nível de um certificado e o emissor do certificado raiz for confiável, este certificado é confiável.

**Importante:**

Lembre que a chave privada raiz e o certificado raiz (ca.key e ca.pem) **DEVEM** ser armazenados em local seguro.

Na próxima seção, será criado um certificado filhos emitidos pelo certificado raiz. 

Alguem que esteja solicitando um certificado, o faz ao emissor usando o arquivo `.csr`.

#### Passo 2 - Gerar a chave privada e o certificado para o servidor (Server Certificate)
Suponha que queremos criar uma chave privada e um certificado para qualquer domínio, digamos, `ec2-3-80-250-87.compute-1.amazonaws.com`. 

Primeiro, precisamos criar uma chave privada para o servidor.

assim como antes:

```
openssl genrsa -out techday.key 2048
```

então, criamos um arquivo de configuração `openssl.cnf`

- `req_distinguished_name` ：modifique de acordo com a situação

```
[req]
default_bits  = 2048
distinguished_name = req_techday
req_extensions = req_ext
x509_extensions = v3_req
prompt = no
[req_techday]
countryName = BR
stateOrProvinceName = São Paulo
localityName = São Paulo
organizationName = TCT
commonName = ec2-3-80-250-87.compute-1.amazonaws.com
[req_ext]
subjectAltName = @alt_names
[v3_req]
subjectAltName = @alt_names
[alt_names]
IP.1 = 3.80.250.87
DNS.1 = ec2-3-80-250-87.compute-1.amazonaws.com
```

>ps: **commonName** DEVE ser o mesmo que dns.   
 
Então, use esta chave e configuração para emitir uma **solicitação de certificado**:

```
openssl req -new -key ./techday.key -config openssl.cnf -out techday.csr
```

agora, o `techday.csr` pode ser enviado para o administrador que tem a chave raiz e certificado para solicitar o techday.pem (o certificado techday).

#### Passo 3 - Emitir o certificado para o servidor (Server Certificate)
> Este passo será executado pelo administrador que tem o certificado raiz e a chave.

O admin usa o certificado raiz para emitir o certificado de techday usando o seguinte comando:

```
openssl x509 -req -in ./techday.csr -CA ca.pem -CAkey ca.key -CAcreateserial -out techday.pem -days 3650 -sha256
```
- `openssl x509`: Este comando é usado para manipular certificados X.509, que são o formato padrão para certificados de chave pública.
    
- `-req`: Esta opção especifica que o arquivo de entrada (`./techday.csr`) é uma solicitação de certificado.
    
- `-in ./techday.csr`: Esta flag especifica o arquivo CSR de entrada que você quer assinar e transformar em um certificado. O `./techday.csr` é o caminho para o arquivo CSR.
    
- `-CA ca.pem`: Esta opção especifica o arquivo de certificado CA (Autoridade de Certificação) que será usado para assinar o CSR. Neste caso, `ca.pem` é o arquivo de certificado CA.
    
- `-CAkey ca.key`: Esta opção especifica a chave privada do certificado CA (`ca.key`) usada para assinar o certificado. Esta chave privada é necessária para realizar a operação de assinatura com segurança.
    
- `-CAcreateserial`: Esta opção instrui o OpenSSL a criar um arquivo de número de série para o certificado. O número de série é um identificador único para cada certificado emitido pela CA.
    
- `-out techday.pem`: Isto especifica o arquivo de saída onde o certificado assinado será escrito. Neste caso, `techday.pem` é o arquivo de saída onde o certificado assinado será salvo.
    
- `-days 3650`: Esta opção define o período de validade do certificado em dias. Neste caso, o certificado será válido por 3650 dias, o que é aproximadamente 10 anos.
    
- `-sha256`: Esta opção especifica o uso do algoritmo de hash SHA-256 para a assinatura digital. O algoritmo SHA-256 fornece segurança forte e é comumente usado para assinaturas de certificados.

Finalmente, o `techday.pem` pode ser usado para configurar o broker MQTT para habilitar a comunicação segura usando TLS/SSL.

