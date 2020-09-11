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

## Marmara Kredi Döngüleri
 
Mevcutta Marmara Kredi Döngüleri %100 karşılıksızlığı önleyen modda protokol 1 üzerine çalışmaktadır. %100 karşılıksızlık  colls made by issuer on behalf of both himself/herself and holder. Both issuer and holder have the 3x staking chance to get blockchain rewards. Issuer has the 3x staking chance until maturity date of credit whereas holder has the 3x staking chance until he/she endorses/transfers the credit to a new holder who will continue staking with the issuer. 
The Credit loops can be made using only activated coins.
### Kavramlar

**Keşideci:** Krediyi ilk çıkaran ve kredi döngüsünü başlatan kişi.

**Hamil:**  Kredi Döngüsündeki son düğüm. Keşideci başta olmak üzere diğer tüm düğümler kredi vadesi dolduğunda hamil olan düğümün borcunu ödemekle mükelleftir. Hamil dışında hepsi döngüde alışveriş sırasında mal ve hizmet almışlardır. Ancak hamil mal ve hizmet sağladığı halde elinde sadece kredi mevcuttur. İşte diğer tüm düğümler bu mal ve hizmetin karşılığı olan parayı hamil olan düğüme öderler.

**Ciranta:** Kredi Döngüsündeki ilk düğüm olan Keşideci ile son düğüm olan hamil arasında kalan tüm düğümler Ciranta yaptıkları işlem ise cirolama olarak adlandırılır. Cirantalar mal ve hizmet karşılığında krediyi bir sonraki düğüme aktarırlar.

**Vade:** Mevcut bloktan başlayarak kredinin vadesinin kaç blok sonra bittiğini göstermektedir. Vade blokların çıkışına göre hesaplanır. Marmara zincirinde bir günde çıkan blok sayısı 1440'tır (Bir Saatte 60 blok çarpı 1 günde 24 saat sonucu). 100 günlük bir kredi için, Vade 1440x100 = 144,000 blok şeklinde hesaplanır.

**Settlement:** Bir kredinin vadesi dolduğunda, ödeme döngüde son düğüm olan hamile aktarılır. Settlement may be automatic or manual.

**Escrow:** Trust based parties for Protocol 2. If EscrowOn is false, then 100% collateralization is used and settlement is automatic. There is no need for escrows in protocol 1 which works as complete trustless version.

**Avalist:** Avalists support issuer or endorsers with MCL as additional colletarization and can earn with 3x staking with those support. In case of non-redemption, their funds are utilized. Avalists are available only in Protocol 2. The parameter avalcount is always zero for protocol 1.

**BlockageAmount:** This is a term for protocol 2. An issuer may be asked to put some collateralization by a holder. In that case, the issuer gets benefit of 3x staking in protocol 2.

**Dispute Expiry:** It is grace period for solving non-redemption problem in credit loops in protocol 2. An issuer may have this time as blocks when creating a credit under protocol 2 without or insufficient collateralization. Before this period expires, an escrow should do all actions according to aggrement with the issuer to solve non-redemption. Otherwise, the escrow is penalized in the system.

### Kredi Döngüsü Oluşturmak için Önemli Komutlar
Marmara Kredi Döngüsünü gerçekleştirebilmek oldukça basittir. Sadece 4 komutu bilmeniz yeterlidir. Ancak kredi döngüsü var edebilmek veya bir sonraki düğüme aktarabilmek için yeterli aktif/kilitli koininiz olması gerekir. Bunun için kilitleme komutlarını kullanabilirsiniz.

Kredi döngüsü başlatma ve aktarma işlemlerini gerçekleştirmek için eşler (p2p) karşılıklı birbirinin public key adresini bilmek zorundadırlar.

- ```marmararecieve```

Kredi talebinde bulunma komutu. Ciranta ve hamil durumda olanlar kullanırlar.


**Birinci Senaryo:** İki düğüm ilk defa bir Marmara Kredi döngüsü başlatacaklardır. Bu döngü mal veya hizmet karşılığında başlatılacak olabilir. Bu durumda, Hamil (mal veya hizmeti satan, alacaklı kişi), Keşideciye (mal veya hizmeti satın alacak kişiye) kredi talebinde bulunur. Bu talep aşağıdaki komutun, komut satırına yazılması ile gerçekleşir: 
```
./komodo-cli -ac_name=MCL marmarareceive senderpk amount currency matures '{"avalcount":"n"}'
```
>```senderpk``` Keşidecinin (mal veya hizmeti satın alacak kişinin) pubkey adresidir.
>
>```amount``` ödeme miktarını belirtir. Bu miktar Keşidecinin aktif hesabında yer alan miktardır. Eğer aktif hesapta belirtilen miktarda koin yoksa bu durumda ```marmaralock``` komutu ile Keşideci tarafından normal hesabından aktifleştirilmesi gerekmektedir.
>
>```currency``` ödeme birimi olan MARMARA'dır.
>
>```matures``` mevcut bloktan başlayarak kredinin vadesinin kaç blok sonra bittiğini göstermektedir. Bir saatte çıkan blok sayısı 60 olup, 24 saatlik zaman diliminde çıkan toplam blok sayısı 1440'tır. Kredi bitiş süresi buna göre hesaplanıp bu parametreye karşılık olarak girilmelidir.
>
>```'{"avalcount":"n"}'``` avalistlerin sayısıdır. Marmara Kredi Döngülerinin birinci protokülü bazında avalist sayısı sıfıra eşittir. Örnekle, bu parametre için sıfır değeri verilmelidir. '{"avalcount":"0"}'

Yukarıda verilen komut sonuç olarak bir hex kodu oluşturup döndürür. BU hex kodunun Hamil tarafından onaylanması gerekir, bunun 
This marmarareceive call generates a hex code. This HEXCODE needs to be verified by the holder by executing the ```sendrawtransaction``` command:
```
./komodo-cli -ac_name=MCL sendrawtransaction HEXKODU

```
Bu komutun çalıştırılmasının ardından, ```txid``` şeklinde bir işlem numarası oluşturulur. Kredi döngüsünün tamamlanması için bu işlem numarası, ```txid```, ile Hamilin pubkey adresi,```receiverpk```, Keşideciye iletilmelidir. Bu iletişime alternatif olarak ```marmarareceivelist``` komutu da kullanabilir. Bu komut ile Keşideci kendisine Hamil tarafından gelen isteklerin listesini görüntüleyebilir. Bu komutun kullanımı aşağıda verilmiştir.

```marmarareceivelist``` komutu Keşideci tarafından aşağıdaki şekilde kullanılır:
```
./komodo-cli -ac_name=MCL marmarareceivelist pubkey

```
> Burada yer alan ```pubkey``` Keşidecinin Marmara zincirine bağlı olan pubkey adresini ifade eder.
>Bu komut ilgili pubkey tarafından oluşturulan işlem numaralarının listesini döndürür.

- ```marmaraissue```

```marmaraissue``` Sadece ilk düğüm noktası tarafından krediyi ilk var etmek yani kredi döngüsünü başlatmakta kullanılacak komuttur. Bu komutla kredi var edilir ve döngüdeki 2. Düğüme yani ilk hamile aktarılır. Kredi döngüsüne ait koşullar Keşideci ile Hamil arasında belirlenmektedir. 
 
Kredi döngüsünün koşullarını içeren komut aşağıda yer almaktadır:

```
./komodo-cli -ac_name=MCL marmaraissue receiverpk '{"avalcount":"n", "autosettlement":"true"|"false", "autoinsurance":"true"|"false", "disputeexpires":"offset", "EscrowOn":"true"|"false", "BlockageAmount":"amount" }' requesttxid
```
>```receiverpk``` krediyi alacak olan Hamilin pubkey adresidir.
>
>```"avalcount":"n"``` avalistlerin sayısıdır. Marmara Kredi Döngüleri Protokol 1 kapsamında bu parametre için sıfır değeri verilmelidir.
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

 Keşideci tarafından kredi döngüsnün tamamlanabilmesi için ```marmaraissue``` yapılmalıdır. Bu komutun tipik bir örneği aşağıda veilmiştir:
```
./komodo-cli -ac_name=MCL marmaraissue receiverpk '{"avalcount":"0", "autosettlement":"true", "autoinsurance":"true", "disputeexpires":"0", "EscrowOn":"false", "BlockageAmount":"0" }' requesttxid
```
```marmaraissue``` komutu karşılığında bir hex kodu döndürür ve sonrasında Keşideci sendrawtransaction komutunu kullanarak bu işlemin blokzincirinde aşağıdaki şekilde işletilmesini sağlamalıdır:
```
./komodo-cli -ac_name=MCL sendrawtransaction HEXCODE

```
Böylelikle keşideci ile hamil arasında ilk döngü oluşturulmuş olur. Döngüde kilitlenen krediler mal veya hizmet alım sirkülasyonunda kullanılabilirler. Hamil ve keşideci bu şekilde döngünün vade süresi dolana kadar 3 kat staking yapma şansını yakalabilirler.
- ```marmaracreditloop```

Hamil ile keşideci arasında gerçekleştirilen kredi döngüsünü gösterebilmek için ```marmaracreditloop``` komutu kullanılabilir. Komutun kullanımı aşağıda verilmektedir:
```
./komodo-cli -ac_name=MCL marmaracreditloop txid
```
> ```txid``` Marmara Kredi Döngüsüne ait olan baton transfer id'sini verir.

- ```marmaratransfer```

**İkinci Senaryo:** Bir önceki senaryoda yer alan Hamil döngüde kilitlediği koinleri mal ve hizmet alarak değerlendirmek istemektedir. Bu durumda, Hamil Ciranta rolüne geçmektedir ve kredi döngüsünde son düğüme her zaman Hamil denilmektedir. Diğer bir deyişle bütün cirantalar önceki durumda hamil rolünü alırlar.
Burada dikkate alınması gereken nokta cirantalar krediyi Hamile transfer ettikleri noktada 3 kat staking gücünü kaybetmeleri olacaktır.

Ciranta krediyi bir sonraki düğüme aktarmak için ```marmarareceive``` komutunu kullanır. Ciranta bu durumda artık hamil değildir.

```
./komodo-cli -ac_name=MCL marmarareceive senderpk batontxid '{"avalcount":"n"}'
```
>```senderpk``` cirantanın pubkey adresidir.
>```batontxid``` bir önceki kredi döngüsünde çıkan baton işlem numarasıdır.
>```'{"avalcount":"n"}'``` avalistlerin sayısını belirtir. Protokol 1 için '{"avalcount":"0"}' şeklinde verilmelidir.

Bu marmarareceive komutu bir hex kodu üretir ve HEX KODU ```sendrawtransaction``` altında blokzincire ilgili işlemi kaydetmek için aşağıdaki şekilde gönderilir:
```
./komodo-cli -ac_name=MCL sendrawtransaction HEXKODU

```
Bu komutun işlenmesinin ardından bir baton işlem numarası,```txid```, otomatik olarak oluşturulur. İşbu kredi döngüsünü tamamlamak için baton numarasının, ```txid```, ve yeni hamilin pubkey adresinin, ```receiverpk```, cirantaya iletilmesine ihtiyaç vardır. Fakat buna alternatif olarak ```marmarareceivelist``` komutu ciranta tarafından kullanılarak kendisine gelen istekler görüntülenebilir.

Sonrasında, ciranta aşağıda yer alan ```marmaratransfer``` komutunu kullanarak kredinin yeni hamile geçmesini sağlayabilir:
```
./komodo-cli -ac_name=MCL marmaratransfer receiverpk '{"avalcount":"n"}' requesttxid
```
>```receiverpk``` yeni hamilin pubkey adresidir. 
>
>```"avalcount":"n"``` avalistlerin sayısını belirtir. Protokol 1 için '{"avalcount":"0"}' şeklinde verilmelidir.
>
>```requesttxid``` yeni hamil tarafından oluşturulan baton işlem numarasıdır.

İlgili işlemin blokzincirine kaydedilmesi için ciranta ```sendrawtransaction``` komutu ile yukarıdaki ```marmaratransfer``` komutunun döndürdüğü HEX KODUNU onaylar.

>**ÖNEMLİ NOT:** ```sendrawtransaction``` komutu ```marmarareceive```, ```marmaraissue```, ```marmaratransfer``` ve ```marmaralock``` komutlarının kullanımınından hemen sonra mutlaka kullanılmalıdır. Bunun amacı bu komutların sonuçlarının blokzincirinde işletilmesini sağlamaktır. Kullanımına yönelik örnekler Senaryo 1 ve Senaryo 2'de verilmektedir.

**Bu şekilde, belirlenen _vaad süresi_ boyunca kredi döngüleri bininci düğüme kadar mal ve hizmet karşılığında ekonomide sirküle edilebilir.**


## Cüzdanın Yedeğinin Alınması

Varlıkların tutulduğu cüzdanın (`wallet.dat` dosyasının) yedeğinin alınması yüksek önem arz etmektedir. 

Linux makinasında ilgili dosya `~/.komodo/MCL/wallet.dat` uzantısında yer alabilir.

Yedek alma yöntemlerinden biri bu dosyanın bir kopyasının arşivini almaktır. 

Bunun için aşağıdaki 

```bash
#wallet.dat dosyasının kopyası alınır
cp -av ~/.komodo/MCL/wallet.dat ~/wallet.dat

#wallet.dat dosyası yeniden adlandırılır
mv ~/wallet.dat ~/2020-08-09-wallet_backup.dat

# wallet.dat dosyasının arşivi alınır
tar -czvf ~/2020-08-09-wallet_backup.dat.tgz ~/2020-08-09-wallet_backup.dat

# İlgili dosya güvenilir bir alana taşınır
```

### **NOT : [ Marmara Credit Loops Windows Kurulum ve Kullanım.](https://github.com/marmarachain/MarmaraChain/blob/master/win-mcl.md)**


Kaynakça
---
Komodo Antara Çerçevesi ve detayları üzerine linkte yer alan [geliştirici dokümantasyonundan](https://developers.komodoplatform.com/) faydalanabilirsiniz.

Marmara Kredi Döngülerinin çalışma prensibi üzerine linkte verilen [makaleden](https://medium.com/@drcetiner/marmara-kredi-d%C3%B6ng%C3%BCleri-nas%C4%B1l-%C3%A7al%C4%B1%C5%9F%C4%B1r-201144573e7c) faydalanabilirsiniz. 

Lisans
---
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

Yukarıdaki telif hakkı bildirimi ve bu izin bildirimi, Yazılımın tüm kopyalarına veya önemli kısımlarına dahil edilecektir.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.