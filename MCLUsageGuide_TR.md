<<<<<<< Updated upstream
![MarmaraCreditLoops Logo](https://raw.githubusercontent.com/marmarachain/Marmara-v.1.0/master/MCL-Logo.png "Marmara Credit Loops Logo")

# Marmara v.1.0.1 Kullanım Kılavuzu

## Linux İşletim Sistemi
## Başlangıç

Marmara Kredi Döngüleri yazılımı kaynağından indirilip yüklendiyse bu durumda komutlar ```cd komodo/src``` klasöründen çalıştırılmalıdır. Kurulum konfigürasyonuna göre ```komodod``` ve ```komodo-cli``` klasörleri farklı dizilimde yer alabilir, bu sebeple bu klasörler bulunup dizin buna göre değiştirilmelidir.

Marmara zincirini asağıdaki parametrelerle başlatınız:

```
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 &
```
Bağlanıp senkronize olana bekleyiniz. Aşağıdaki komutu kullanarak nodunuzun zincire bağlanıp bağlanmadığını kontrol ediniz:

```
./komodo-cli -ac_name=MCL getinfo
```

**_Blokları Çekme_**

Yeni başlayanlar için bütün blokların çekilmesi ve senkronize olması adımı çok uzun vakit alabilir. Bu durumda, bu süreci hızlandırmak için [bootstrap](https://eu.bootstrap.dexstats.info/MCL-bootstrap.tar.gz) indirilip bu kullanılabilir.

Bunun için öncelikle aşağıdaki komut ile Marmara Zinciri durdurulur:
```	
./komodo-cli -ac_name=MCL stop
```
Komut satırından bootstrap yazılımını indirmek için aşağıdaki komut kullanılabilir:
```
wget https://eu.bootstrap.dexstats.info/MCL-bootstrap.tar.gz
```
Aşağıdaki komut ile bootstrap içerikleri aşağıda verilen dosya dizimi içine çıkarılır:
 
```
tar -xvf MCL-bootstrap.tar.gz -C .komodo/MCL
```
Aşağıdaki komut ile Marmara Zinciri tekrardan başlatılır:
```
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 &
```

## Pubkey Oluşturma ve Marmara Zincirini Pubkey ile Başlatma

Marmara Kredi DÖngülerini kullanabilmek için kullanıcının bir **pubkey'e** sahip olması ve zinciri bu **pubkey** ile başlatması beklenmektedir.
Madencilik ile veya staking ile kazanılan koinlerin bir hesaba gitmesi için pubkey almak bir şarttır, zincir pubkey ile başlatılmadığı takdirde yapılan **madencilik veya staking** eylemleri boşa gidecektir.  

Pubkey oluşturmak için Marmara Zinciri önceden yukarıda verilen komut ile başlatılır ve aşağıdaki [getnewaddress](https://developers.komodoplatform.com/basic-docs/smart-chains/smart-chain-api/wallet.html#getnewaddress) API komutu ile yeni adres oluşturulur:
```	
./komodo-cli -ac_name=MCL getnewaddress
```
Bu komut aşağıdaki örnekte olduğu gibi bir adres döndürmektedir:
```	
DO_NOT_USE_THIS_ADDRESSgg5jonaes1J5L786
```
Sonrasında aşağıdaki [validateaddress](https://developers.komodoplatform.com/basic-docs/smart-chains/smart-chain-api/util.html#validateaddress) komutu çalıştırılır:
```	
./komodo-cli -ac_name=MCL validateaddress DO_NOT_USE_THIS_ADDRESSgg5jonaes1J5L786
```
Yukarıdaki komut sonucunda birden fazla nesnenin barındırıldığı JSON nesne döndürecek olup, pubkey bunlardan birisidir:
```	
"pubkey": "DO_NOT_USE_THIS_ADDRESS019n79b0921a1be6d3ca6f9e8a050mar17eb845fe46b9d756"
```

Komut sonucunda döndürülen kullanıcının MCL pubkey'idir ve sonrasında zincire bu pubkey ile bağlanmak üzere kullanıcı tarafından bu pubkey not edilmelidir.

Bunun için öncelikle zincir aşağıdaki komut ile durdurulur:
```	
./komodo-cli -ac_name=MCL stop
```
Sonrasında başlatma parametrelerinin sonuna aşağıdaki örnekte gösterildiği üzere kullanıcının MCL pubkey'i eklenerek Marmara blokzinciri tekrardan başlatılır:
```	
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 -gen -genproclimit=1 -pubkey=DO_NOT_USE_THIS_ADDRESS019n79b0921a1be6d3ca6f9e8a050mar17eb845fe46b9d756 &
```
> Yukarıdaki komut içerisinde yer alan ```-genproclimit``` madencilikte kullanılacak olan thread sayısını belirlemektedir.

>Eğer ```-genproclimit``` 1'e eşitlendiyse; ```-gen -genproclimit=1```, Marmara blokzinciri tek bir işlemci ile başlatılır. Makinanın çekirdek sayısına bağlı olarak ```-genproclimit``` parametresine 1'den farklı işlemci sayısı da atanabilir.  

>Eğer ```-genproclimit``` -1'e eşitlendiyse; ```-gen -genproclimit=-1```,  Marmara blokzinciri bütün işlemci gücünü kullanacak şekilde başlatılır.

>Eğer ```-genproclimit``` 0'a eşitlendiyse; ```-gen -genproclimit=0```, Marmara blokzinciri staking modu ile başlatılmış olup aktif olan koinler üzerinden staking yapılmaya başlanır.

## dumpprivkey

`dumpprivkey` metodu verilen adresin private anahtarını göstermeye yarayan bir komuttur. 

Örnek olması amacıyla bu metodun kullanımı aşağıdaki komutta gösterilemektedir.

```
./komodo-cli -ac_name=MCL dumpprivkey "PWqwYaWNEVT7V9SdfFHARWnoB7vcpSfdvs"
```
Komut örnekle aşağıdaki örnek private adresi döndürmektedir: 
```
DONOTUSETHISxxxxxxxxxxxxxxxxx7KkCmRnnSg7iXvRUqhYoxC9Y
```

## importprivkey

`importprivkey` metodu `dumpprivkey` metodu tarafından döndürülen private adresin cüzdan adresine eklenmesini sağlamaktadır. Bu komut [burada](https://developers.komodoplatform.com/basic-docs/smart-chains/smart-chain-api/wallet.html#importprivkey) gösterildiği üzere birden fazla argümanla çalıştırılabilir.

Komutun en basit hali aşağıda örnekle verilmektedir:
```
./komodo-cli -ac_name=MCL importprivkey "DONOTUSETHISxxxxxxxxxxxxxxxxx7KkCmRnnSg7iXvRUqhYoxC9Y"
```
Bahsi geçen komut örnek olarak aşağıdaki cüzdan adresini döndürmektedir:
```
R9z796AehK5b6NCPeVkGUHSpJnawerf8oP
```
Yukarıda döndürülen cüzdan adresini aşağıda verilen ```validateaddress``` ile doğrulayarak MCL pubkey adresine erişim sağlanabilir:
```
./komodo-cli -ac_name=MCL validateaddress R9z796AehK5b6NCPeVkGUHSpJnawerf8oP
``` 
Bu yöntem ile alınan MCL pubkey adresi ile Marmara blokzinciri başlatılabilir.

## Marmara Blokzincirini Madencilik veya Staking Modunda Çalıştırma

Aşağıdaki komut ile makinenizda kurulu olan Marmara zincirinin çalışma modu kontrol edilebilmektedir:

```	 
./komodo-cli -ac_name=MCL getgenerate
```
Yukarıdaki komut çalıştırıldığında, aşağıdaki örnekteki gibi JSON tipinde nesne döndürülür:
```	
{
  "staking": false,
  "generate": true,
  "numthreads": 1
}
```
Yukarıdaki çıktıdan görüldüğü üzere, ilgili nod madencilik modunda çalışmaktadır.
>```"staking": false``` staking modunun kapalı olduğunu belirtir.
> ```"generate": false``` madencilik modunun açık olduğunu belirtir.
>```"numthreads": 1``` madencilik sırasında kullanılan thread'lerin sayısını vermektedir. (Önceden -gen -genproclimit=1 komutu ile 1 olarak belirlendiği için)
>
İlgili nodun modunu **staking** olarak aktif etmek için aşağıdaki komut kullanılmaktadır:
```	   
 ./komodo-cli -ac_name=MCL setgenerate true 0
```
Nodun çalışma modunu tekrardan kontrol etmek için aşağıdaki komut komut satırına yazıldığında:
```
 ./komodo-cli -ac_name=MCL getgenerate
```
**Staking** modunun aktif olduğu aşağıdaki döndürülen sonuç ile gözlemlenir:
```
{
  "staking": true,
  "generate": false,
  "numthreads": 0
}
```
> Dikkat edilmesi gerek nokta koinlerinizi aktif hale getirmediyseniz; ```"staking": true``` ile _**staking modu aktif olsa dahi staking yapılamaz!**_

## Adrese Koin Gönderme İşlemi
Aşağıdaki komut ile **normal koinlerinizden (NormalCoin)** dilediğiniz bir adrese direk olarak ödeme yapabilirsiniz. Ödeme miktarı en yakın 0.00000001 sayısına yuvarlanmaktadır. Buna ek olarak bu yöntem ile gönderim yapıldığında hesabınızdan gönderim ücreti kesimi yapılır.

```
./komodo-cli -ac_name=MCL sendtoaddress "MCL_address" amount
```
> **Önemli Not:** Bu komut kullanıcı tarafından belirtilen adrese direk olarak ödeme yapılmasını sağlamakta olup, bu ödeme sonrasında geri çekilemez.


## Marmara Zinciri ve Cüzdan Bilgilerine Erişim için Önemli Komutlar
Aşağıda yer alan komutlar zincire bağlanan cüzdanınınız ve zincirle ilgili önemli ayrıntılara erişim için kullanılmaktadır. 

- ```getinfo``` 
```
./komodo-cli -ac_name=MCL getinfo
```

```getinfo``` komutu Marmara Zinciri ile ilgili önemli bilgileri göstermektedir. Döndürülen parametrelerden ```"version"``` MARMARA'nın versiyonunu, ```synced``` zincire bağlanan makinenin senkronizasyon durumunu (Senkronizasyonun sağlandığı bilgisi "blocks" ve "longestchain" parametrelerinin değerlerinin eşit olmasından anlaşılmaktadır), ```"difficulty"``` ile zincirin zorluk değeri, ```"connections"``` parametresi ile zincire bağlı olan makinenize en yakın zincirdeki diğer nodların sayısını, ```"pubkey":``` ile zincire bağlı olan pubkey gösterilmektedir.

- ```getpeerinfo``` 
```
./komodo-cli -ac_name=MCL getpeerinfo  
```
```getpeerinfo```  komutu, Marmara Zincirine bağlı olan makinenize en yakın zincire bağlı nodların detay bilgilerini gösterir.  
- ```marmarainfo```  
```
./komodo-cli -ac_name=MCL marmarainfo 0 0 0 0 PubkeyAdresiniz
```
```marmarainfo``` komutu ayrıntılı cüzdan incelemede kullanılmakta olup,  ```"myPubkeyNormalAmount"``` parametresi cüzdanınızda bulunan normal MARMARA miktarını;  ```"myActivatedAmount"``` parametresi aktif MARMARA koin miktarını; ```"Loops"``` parametresi gerçekleştirilen kredi döngülerini; ```"TotalLockedInLoop"``` parametresi kredi döngülerinde kilitlenen toplam MARMARA koin miktarını; ```"numclosed"``` parametresi toplamda kapatılan kredi döngüsü sayısını; ve ```"closed"``` parametresi kapatılan kredi döngülerinin detaylarını vermektedir.

##Activating and Deactivating Coins

- ```marmaralock``` is used to activate the coins. Active coins are needed for staking and if there are none then even if the staking mode is on, no blocks would be found through staking. The entire command is given below and the **amount** is to be replaced by the amount of coins such as 1000.  
```  
./komodo-cli -ac_name=MCL marmaralock amount
```
This command in turn returns a JSON object with the result of the operation and the hex created for it. Note that the _**"hex"**_ given below is for demonstration purposes.
```
{
  "result": "success",
  "hex": "0400008085202f89020039b219200ae4b5c83d77bffce7a8af054d6fb..........e9181f6aac3e1beb1e260e9a1f49ed24e6ac00000000edeb04000000000000000000000000"
}
```
Now, in order to confirm this transaction, copy the hex returned through the JSON object and validate it through the ```sendrawtrasaction``` command given below.
```  
./komodo-cli -ac_name=MCL sendrawtranscation 0400008085202f89020039b219200ae4b5c83d77bffce7a8af054d6fb..........e9181f6aac3e1beb1e260e9a1f49ed24e6ac00000000edeb04000000000000000000000000
```
If the avove command gets successfully executed in the blockchain, it gives out a transaction id in response. One may check if this transaction is verified by searching the respective id in the [Marmara Explorer site](http://explorer.marmara.io).
To see the activated coins, use ```marmarainfo``` command provided earlier and search for the value across the ```"myActivatedAmount"``` parameter. Note that the raw transactions are collected in the mempool and a few blocks may be needed to found to see the transaction recorded on the block.


- ```marmaraunlock``` is used deactivate the coins i.e. turn them into normal amount. Normal Amount is utilized for sending payments directly to an address through```sendtoaddress``` command explained earlier. The **amount** is to be replaced by the amount of coins to be deactivated such as 500.   
```  
./komodo-cli -ac_name=MCL marmaraunlock amount
```
In the same way explained earlier, this transaction needs to be validated through the ```sendrawtrasaction``` command given above. For this purpose, copy the hex returned by ```marmaraunlock``` command and use it with ```sendrawtrasaction``` command.
  
- ```listaddressgroupings``` is used to list the pairs of wallet addresses and respective normal amounts in them. The usage is given in the command below.
```
./komodo-cli -ac_name=MCL listaddressgroupings
```

## How to Make Marmara Credit Loops
The current Marmara Credit loops currently work based on Protocol 1 which is in 100% collateralization mode. 100 % collateralization is made by issuer on behalf of both himself/herself and holder. Both issuer and holder have the 3x staking chance to get blockchain rewards. Issuer has the 3x staking chance until maturity date of credit whereas holder has the 3x staking chance until he/she endorses/transfers the credit to a new holder who will continue staking with the issuer. 
The Credit loops can be made using only activated coins.
### Terminology
**Issuer:** The person who first creates a credit in a credit loop. It is the person who forms the first node in the credit loop. The credit may be collateralized 100% or with even zero-collateralization until maturity date of a credit.

**Bearer (Holder):** The last node in a credit loop is always called bearer (holder). When someone transfers a credit in a loop, that node becomes immediately an endorser.

**Endorser:** All other nodes that fall between the issuer, which is the first node in the credit loop, and the last node, and transfer the credit to the next node.

**Maturity:** The time a credit expires. It is measured as blocks in estimation. The block per day is 1440 (60 blocks an hour times 24 hours a day). Suppose a credit is for 100 days, the maturity is 1440x100, i.e. 144,000 blocks.

**Settlement:** When a credit matures, settlement is made to the holder, the last node in a loop. Settlement may be automatic or manual.

**Escrow:** Trust based parties for Protocol 2. If EscrowOn is false, then 100% collateralization is used and settlement is automatic. There is no need for escrows in protocol 1 which works as complete trustless version.

**Avalist:** Avalists support issuer or endorsers with MCL as additional colletarization and can earn with 3x staking with those support. In case of non-redemption, their funds are utilized. Avalists are available only in Protocol 2. The parameter avalcount is always zero for protocol 1.

**BlockageAmount:** This is a term for protocol 2. An issuer may be asked to put some collateralization by a holder. In that case, the issuer gets benefit of 3x staking in protocol 2.

**Dispute Expiry:** It is grace period for solving non-redemption problem in credit loops in protocol 2. An issuer may have this time as blocks when creating a credit under protocol 2 without or insufficient collateralization. Before this period expires, an escrow should do all actions according to aggrement with the issuer to solve non-redemption. Otherwise, the escrow is penalized in the system.

###Important Commands for Making Credit Loops
- ```marmararecieve```

This command is used to get a credit from an issuer or an endorser. When asking a credit from an issuer, i.e. the first node, it has a unique use. In other nodes, it is the same.

**Scenario 1:** Two nodes are making a credit loop for the first time. This credit loop may be created for a sale of a good or service in the market. In such case, the holder (the one selling the product/service) should request for a credit from the issuer (the one paying the product/service) by writing down the following command:
```
./komodo-cli -ac_name=MCL marmarareceive senderpk amount currency matures '{"avalcount":"n"}'
```
>```senderpk``` is the pubkey address of the issuer (the one paying the product/service)
>
>```amount``` is the payment amount. Please note that this amount should be available in activated fund of the issuer and if not then must be activated thru ```marmaralock``` command by th issuer.
>
>```currency``` is MARMARA
>
>```matures``` is the time that respective credit expires, 60 blocks an hour times 24 hours a day making 1440 blocks per day.
>
>```'{"avalcount":"n"}'``` is the number of avalists i.e. '{"avalcount":"0"}' for protocol 1. 


This marmarareceive call generates a hex code. This HEXCODE needs to be verified by the holder by executing the ```sendrawtransaction``` command:
```
./komodo-cli -ac_name=MCL sendrawtransaction HEXCODE

```
Once this command is executed, a transaction id named ```txid``` gets generated. This ```txid``` along with the ```receiverpk``` needs to be communicated to the issuer to complete the credit loop. But, an alternative to this communication would be the use of ```marmarareceivelist``` method which could be used to see the receive requests made to the issuer himself/herself.

```marmarareceivelist``` method would be executed by the issuer by the following command:
```
./komodo-cli -ac_name=MCL marmarareceivelist pubkey

```
> ```pubkey``` is the pubkey address of the issuer connected to the Marmara Chain
>The response of this command is a list of pair of txid's created by the respective pubkeys.

- ```marmaraissue```

This command is only used by issuer, the first node to create/issue a credit. By this, a credit is also transferred to the first holder, i.e. the second node. Many of the parameters for a credit loop is decided between the issuer and the first holder.
```marmaraissue``` method takes in the following arguments: 
```
./komodo-cli -ac_name=MCL marmaraissue receiverpk '{"avalcount":"n", "autosettlement":"true"|"false", "autoinsurance":"true"|"false", "disputeexpires":"offset", "EscrowOn":"true"|"false", "BlockageAmount":"amount" }' requesttxid
```
>```receiverpk``` is the pubkey of the receiver which is the holder.
>
>```"avalcount":"n"``` is the number of avalists i.e. '{"avalcount":"0"}' for protocol 1. 
>
>``` "autosettlement":"true"|"false"``` AutoSettlement is true due to 100% collateralization in Protocol 1.
>
>```"autoinsurance":"true"|"false"``` Autoinsurance is true due to 100% collateralization in Protocol 1.
>
>```"disputeexpires":"offset"```  Dispute expiry is set to 0 due to 100 collateralization in Protocol 1.
>
>```"EscrowOn":"true"|"false"``` EscrowOn is set to false due to 100% collateralization in Protocol 1.
>
>```"BlockageAmount":"amount" }``` blockage amount is set to 0 due to 100 collateralization in Protocol 1.
>
>```requesttxid``` is the txid generated by the holder communicated to the issuer. 

A typical example of a ```marmaraissue``` command to complete the credit loop by the issuer is given below:
```
./komodo-cli -ac_name=MCL marmaraissue receiverpk '{"avalcount":"0", "autosettlement":"true", "autoinsurance":"true", "disputeexpires":"0", "EscrowOn":"false", "BlockageAmount":"0" }' requesttxid
```
This ```marmaraissue``` command in turn returns a hex code response, and now the issuer has to execute the sendrawtransaction method to get the transaction executed on the blockchain as follows:
```
./komodo-cli -ac_name=MCL sendrawtransaction HEXCODE

```
This creates a credit loop between the issuer and the holder. The credits locked in a loop can be circulated to buy things during shopping. The issuer and the holder get 3 times of chances of staking on the MCL funds until the maturity of the credit loop.
- ```marmaracreditloop```

To display the credit loop between the issuer and the holder, the following ```marmaracreditloop``` command may be executed:
```
./komodo-cli -ac_name=MCL marmaracreditloop txid
```
> ```txid``` is the baton transfer id of the Marmara Credit Loop.

- ```marmaratransfer```

**Scenario 2:** The holder from the previous scenario wishes to utilize the coins locked in loop by buying goods/services on the same credit loop created earlier. For such case, when the holder transfers a credit in a loop, that node immediately becomes an endorser. And in such way, the last node in a credit loop is always called the bearer (holder). In other words, all endorsers are previously holders.
One should bear in mind that endorsers lose the 3x staking power when a credit is transferred to a new holder.

For this purpose, the new holder makes a ```marmarareceive``` request to the endorser to get the credit for selling the goods/services by the following command:
```
./komodo-cli -ac_name=MCL marmarareceive senderpk batontxid '{"avalcount":"n"}'
```
>```senderpk``` is the pubkey address of the endorser (the one buying the product/service)
>```batontxid``` is the baton transaction id of the previously created credit loop
>```'{"avalcount":"n"}'``` is the number of avalists i.e. '{"avalcount":"0"}' for protocol 1.

This marmarareceive call generates a hex code. This HEXCODE needs to be verified by the new holder by executing the ```sendrawtransaction``` command:
```
./komodo-cli -ac_name=MCL sendrawtransaction HEXCODE

```
Once this command is executed, a transaction id named ```txid``` gets generated. This ```txid``` along with the ```receiverpk``` needs to be communicated to the endorser to complete the credit loop. But, an alternative to this communication would be the use of ```marmarareceivelist``` method which could be used by the endorser to see the receive requests made to himself/herself.
Then, the endorser executes the following ```marmaratransfer```command to get the credits transferred to the new holder:
```
./komodo-cli -ac_name=MCL marmaratransfer receiverpk '{"avalcount":"n"}' requesttxid
```
>```receiverpk``` is the pubkey of the receiver which is the new holder.
>
>```"avalcount":"n"``` is the number of avalists i.e. '{"avalcount":"0"}' for protocol 1. 
>
>```requesttxid``` is the txid generated by the new holder communicated to the endorser. 

Then the endorser executes the ```sendrawtransaction``` command with the hex code resulting from ```marmaratransfer```command.

>Please Note that ```sendrawtransaction``` command is used after ```marmarareceive```, ```marmaraissue```, ```marmaratransfer``` and ```marmaralock``` to make the results of commands to be executed on blockchain. The usage is presented throughout the scenario 1 and 2.

**In this way, the credit loops can circulate up to 1000th node _within_ the maturity time to buy goods/services.**


## Taking Backup of Wallet

Backing up the `wallet.dat` file is very essential as it holds the assets of one.
On a Linux machine, the file could be found in: `~/.komodo/MCL/wallet.dat`

One method to backup this file is to archive a copy of the file.

```bash
#Copy the wallet.dat file
cp -av ~/.komodo/MCL/wallet.dat ~/wallet.dat

#Rename the wallet.dat file
mv ~/wallet.dat ~/2020-08-09-wallet_backup.dat

# Make an archieve of the wallet.dat file
tar -czvf ~/2020-08-09-wallet_backup.dat.tgz ~/2020-08-09-wallet_backup.dat

# Move the final file to a secure location
```

### **NOT : [ Marmara Credit Loops Windows Kurulum ve Kullanım.](https://github.com/marmarachain/MarmaraChain/blob/master/win-mcl.md)**


References
---
For more detailed information on Komodo Antara Framework and its details, please refer to its extended [developer documentation](https://developers.komodoplatform.com/).

For more detailed information on how Marmara Credit Loops work, kindly refer to detailed article [here](https://medium.com/@drcetiner/how-marmara-credit-loops-mcl-work-31d1896190a5).

License
---
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.