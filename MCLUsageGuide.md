<<<<<<< Updated upstream
![MarmaraCreditLoops Logo](https://raw.githubusercontent.com/marmarachain/Marmara-v.1.0/master/MCL-Logo.png "Marmara Credit Loops Logo")

# Marmara v.1.0.1 Usage Guide

## Getting Started

If you have downloaded and build MCL from source then you can run the commands under ```cd komodo/src``` directory. According to your configuration, ```komodod``` and ```komodo-cli```  may be under different directories. Hence, find where they are and change directory accordingly.

Launch the Marmara Chain with the following parameters:
```
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 &
```
Wait until it connects and synchronizes. You may check if the node sychronized to the chain by executing the following: 
```
./komodo-cli -ac_name=MCL getinfo
```
**TO DO: Provide the output of getinfo and explain it**

**TO DO: information about bootstrap may be added here**

## Creating A Pubkey and Launching MCL with pubkey
To use Marmara Credit Loops, a user must have a **pubkey** and launch the chain with a **pubkey**. Otherwise, any mining or staking in the smart chain would be in vain. Since all mined or staked coins will also be sent to this address. 

In order to get a pubkey, launch the Marmara Chain with the normal launch parameters and execute the [getnewaddress](https://developers.komodoplatform.com/basic-docs/smart-chains/smart-chain-api/wallet.html#getnewaddress) API command.
```	
./komodo-cli -ac_name=MCL getnewaddress
```
This will in turn return a new address:
```	
DO_NOT_USE_THIS_ADDRESSgg5jonaes1J5L786
```
Now, execute the [validateaddress](https://developers.komodoplatform.com/basic-docs/smart-chains/smart-chain-api/util.html#validateaddress) command.
```	
./komodo-cli -ac_name=MCL validateaddress DO_NOT_USE_THIS_ADDRESSgg5jonaes1J5L786
```
This will return a json object with many properties. In the properties one can see:
```	
"pubkey": "DO_NOT_USE_THIS_ADDRESS019n79b0921a1be6d3ca6f9e8a050mar17eb845fe46b9d756"
```
**TO DO: Provide the output of validateaddress and explain it**

This will be your MCL pubkey, make sure to note it. You must now indicate it to the daemon.

In order do this, first stop the daemon.
```	
./komodo-cli -ac_name=MCL stop
```
Then relaunch your daemon using the required parameters, and make sure to include your pubkey as an additional parameter. For example:
```	
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 -pubkey=DO_NOT_USE_THIS_ADDRESS019n79b0921a1be6d3ca6f9e8a050mar17eb845fe46b9d756 &
```
 




#### mining dökümlerinize aşağıdaki kodları kullanarak ulaşabilirsiniz. 

```
./komodo-cli -ac_name=MCL getinfo
./komodo-cli -ac_name=MCL marmarainfo 0 0 0 0 pubkey (to get details)
```

#### Marmara Chaini farklı modlarda çalıştırma  seçenekleri. 

```
-genproclimit=-1 Şayet -1 (yaparsanız tüm işlemci (CPU) günü kullanır.)
-genproclimit=1  Şayet 1  (yaparsanız tek işlemci kullanır.)
-genproclimit=0  Şayet 0  (yaparsanız Staking modunda çalışır ve Aktif coini kullanarak Staking yaparsınız.)
```

#### Not : Sunucu kapanma durumunda yapılacaklar. 

```
cd /komodo/src
./komodo-cli -ac_name=MCL stop
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 -gen -genproclimit=-1 -pubkey="pubkeyburayagirilecek" &
```
---
#### MCL params.
```	
./komodo-cli -ac_name=MCL marmaracreditloop txid
./komodo-cli -ac_name=MCL marmarainfo firstheight lastheight minamount maxamount [currency issuerpk]
./komodo-cli -ac_name=MCL marmaraissue receiverpk amount currency matures approvaltxid
./komodo-cli -ac_name=MCL marmaralock amount unlockht
./komodo-cli -ac_name=MCL marmarapoolpayout perc firstheight "[[\"pubkey\":shares], ...]"
./komodo-cli -ac_name=MCL marmarareceive senderpk amount currency matures batontxid
./komodo-cli -ac_name=MCL marmarasettlement batontxid
./komodo-cli -ac_name=MCL marmaratransfer receiverpk amount currency matures approvaltxid
./komodo-cli -ac_name=MCL marmaraaddress
./komodo-cli -ac_name=MCL marmarainfo 0 0 0 0 <pubkey> //to get details
./komodo-cli -ac_name=MCL walletaddress amount
```

#### Cüzdanınızı yedekleyin

 `wallet.dat` dosyası bütün varlıklarınızın bulunduğu güvenli bir dosyadır. yedeğini almayı unutmatınız!

Linux'ta dosya burada bulunur: `~/.komodo/MCL/wallet.dat`

Bu dosyayı yedeklemenin bir yolu, dosyanın bir kopyasını arşivlemektir.

```bash
# Dosyayı kopyalayın
cp -av ~/.komodo/MCL/wallet.dat ~/wallet.dat

# Dosyayı yeniden adlandır
mv ~/wallet.dat ~/2019-05-17-wallet_backup.dat

# Arşiv yapmak için
tar -czvf ~/2019-05-17-wallet_backup.dat.tgz ~/2019-05-17-wallet_backup.dat

# Son dosyayı güvenli bir yere taşıyıp saklayınız.
```




---
--
#### Start the chain.

Go to src folder.

```
cd ~/komodo/src
```
  
#### Start the chain firstly.

```
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 -pubkey="pubkey to enter here" &
```

#### then create a wallet address and write down this wallet address. 

```
./komodo-cli -ac_name=MCL getnewaddress
```

#### Wallet address sample 
```
RJajZNoEcCRD5wduqt1tna5DiLqiBC23bo
```

#### To confirm this wallet address and then create the pubkey, Text this wallet address in quotation marks , “Enter your wallet address here” secion.
```
./komodo-cli -ac_name=MCL validateaddress "Enter your wallet address here"
```

#### you will get the output like below. and then, write down also the pubkey that written here.
```
{
	": true,
	"address": "RJajZNoEcCRD5wduqt1tna5DiLqiBC23bo",
	"scriptPubKey": "76a914660a7b5c8ec61c59b80cf8f2184adf8a24bccb6b88ac",
	"segid": 52,
	"ismine": true,
	"iswatchonly": false,
	"isscript": false,
	"pubkey": "03a3f641c4679c579b20c597435e8a32d50091bfc56e28303f5eb26fb1cb1eee72",
	"iscompressed": true,
	"account": ""
}
```

this is the your pubkey : `03a3f641c4679c579b20c597435e8a32d50091bfc56e28303f5eb26fb1cb1eee72`

#### Stop the  chain.
```
./komodo-cli -ac_name=MCL stop
```

#### next step is the runing to chain in mining mode by using your own pubkey.

You can run it by using following command. text your pubkey to the area "-pubkey="pubkey to enter here" and then, copy to all command and then, when it is at "cd komodo/src" , paste it and click "enter"
```
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 -gen -genproclimit=-1 -pubkey="pubkey to enter here" &
```

#### at the moment, our server starts to running as mining mode . 

#### you can reach your mining document by using codes below.

```
./komodo-cli -ac_name=MCL getinfo
./komodo-cli -ac_name=MCL marmarainfo 0 0 0 0 pubkey (to get details)
```

#### Options to operate the Marmara Chain in different modes are as follows;

```
-genproclimit=-1 Şayet -1 (If you make -1, all processor (cpu) use.)
-genproclimit=1  Şayet 1  (If you make 1, single processor use.)
-genproclimit=0  Şayet 0  (if you make 0, it run at Staking mode and you can do staking by using active coin.)
```

#### Not : Note: these are what to do in cases of server shutdown,  crashed, reset

```
cd /komodo/src
./komodo-cli -ac_name=MCL stop
./komodod -ac_name=MCL -ac_supply=2000000 -ac_cc=2 -addnode=37.148.210.158 -addnode=37.148.212.36 -addnode=46.4.238.65 -addressindex=1 -spentindex=1 -ac_marmara=1 -ac_staked=75 -ac_reward=3000000000 -gen -genproclimit=-1 -pubkey="pubkey to enter here" &
```
---
#### MCL params.
```	
./komodo-cli -ac_name=MCL marmaracreditloop txid
./komodo-cli -ac_name=MCL marmarainfo firstheight lastheight minamount maxamount [currency issuerpk]
./komodo-cli -ac_name=MCL marmaraissue receiverpk amount currency matures approvaltxid
./komodo-cli -ac_name=MCL marmaralock amount unlockht
./komodo-cli -ac_name=MCL marmarapoolpayout perc firstheight "[[\"pubkey\":shares], ...]"
./komodo-cli -ac_name=MCL marmarareceive senderpk amount currency matures batontxid
./komodo-cli -ac_name=MCL marmarasettlement batontxid
./komodo-cli -ac_name=MCL marmaratransfer receiverpk amount currency matures approvaltxid
./komodo-cli -ac_name=MCL marmaraaddress
./komodo-cli -ac_name=MCL marmarainfo 0 0 0 0 <pubkey> //to get details
./komodo-cli -ac_name=MCL walletaddress amount
```
#### Backup your wallet

We can not stress enough the importance of backing up your `wallet.dat` file.

On Linux, the file is located here: `~/.komodo/MCL/wallet.dat`

One method to backup this file is to archive a copy of the file.

```bash
# Copy the file
cp -av ~/.komodo/MCL/wallet.dat ~/wallet.dat

# Rename file
mv ~/wallet.dat ~/2019-05-17-wallet_backup.dat

# To make archive
tar -czvf ~/2019-05-17-wallet_backup.dat.tgz ~/2019-05-17-wallet_backup.dat

# Move the final file to a secure location
```

#### Now, You are ready to participate in MCL credit loops.


### **NOT : [ Marmara Credit Loops Windows Kurulum ve Kullanım.](https://github.com/marmarachain/MarmaraChain/blob/master/win-mcl.md)**


References
---
For more detailed information on Komodo Antara Framework and its details, please refer to its extended [developer documentation](https://developers.komodoplatform.com/).

License
---
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.