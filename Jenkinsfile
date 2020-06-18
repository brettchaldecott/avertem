import groovy.json.JsonOutput;

def sessionKey=""

pipeline {
   agent any

   environment {
        VERSION_NUMBER = "1.0.${BUILD_NUMBER}"
        MASTER_ACCOUNT_HASH="D594F22DC389E38B3DE7FA5630DBD9DCA16DA8A77097516FD37F9E25C6BE24D2"
        MASTER_PUBLIC_KEY="3082010B0282010200C126A170476DC6E7E4B1A6EE1993F48D8A1413387611BE063B308310AD5B6E26CCBAC841A3ED24823771C0DBDA6613A3A546168A67099CD8B2668A77E8AD8C22C3F1D2560A18B68C44E41DB6646B3C86881E9970B6BAFC8251CFE05D5ECCD6137841E14655A789293D41AD8F2763A7AF6D4ECDACB8A7F62A880A541CCDE68B6CC76EB5BF78600F55D020B80794368287D3514BDB242B31F55A55145A666B56855C3F9B1FF7FC1D5AE2EE87110CABD388BF4527BA3F69D02AF9B7D3471DCF9806BE07919100DF14DE34BB8814697E2CF582DEFE0BD47B8A2D841BECB421A0856A41280E8121A484A807027CD99DD09F2DD1EB44F23C43CDEA59971B2ECC2E8C338F0203010001"
        MASTER_PRIVATE_KEY="308204C1020100300D06092A864886F70D0101010500048204AB308204A70201000282010200C126A170476DC6E7E4B1A6EE1993F48D8A1413387611BE063B308310AD5B6E26CCBAC841A3ED24823771C0DBDA6613A3A546168A67099CD8B2668A77E8AD8C22C3F1D2560A18B68C44E41DB6646B3C86881E9970B6BAFC8251CFE05D5ECCD6137841E14655A789293D41AD8F2763A7AF6D4ECDACB8A7F62A880A541CCDE68B6CC76EB5BF78600F55D020B80794368287D3514BDB242B31F55A55145A666B56855C3F9B1FF7FC1D5AE2EE87110CABD388BF4527BA3F69D02AF9B7D3471DCF9806BE07919100DF14DE34BB8814697E2CF582DEFE0BD47B8A2D841BECB421A0856A41280E8121A484A807027CD99DD09F2DD1EB44F23C43CDEA59971B2ECC2E8C338F02030100010282010136F904349D17C8E742272FD00F7695415D7DB634B921F697F66BA9B9EAE51D562CA9B28A401A23EC6DD451E420E6318BBF63D1D1855EEE859C4CE3F719C19F235BEA6D0770EB34B57A7C045ADF7CC7E6B0422940B3B94759DDE810BEF256D14D0C4A8FCFC15C2405193AA2A79E39D6456F1261265A0DEFD98E0B868206362A137D487DE3F5AFB1828C4FFC0008DA69A1F0088FF2AC92D0BCA0F161A2E08B88E04033182B96BBCF56A9A31F04E909CAA427154393D471FE9A4DAB371EABED51B53AD2E03ABA54102BBA2229F553BD86536C71F09F02E4790EAE5882E52843E6AB6862BBD63BE3238308CCD8C3E508421D532127E55663C3B3235DDFDB5DC449C1A90281810D1541C8EC3758CC70C79F86330055F4A31A8BF4172907477E9E5EFE5F68D48CC6BB6AC6002F427183C6A740EB2E78BFFA7350C8A22E9C5892B22939324D715AF72EADDC41E865C531F812664522BC5985A83E4387173E4BCA9CEAA97DBB87480218FD102998CB6F396AD53F942C295C2391292DF33B1D11C8B5C7212693A4F5F90281810EC3724BD32B257F5CDB46EF904B80A40BFE8F9A2163C948558C38F80DDDD2998857C004E3CE01CB47D7283B20B7E35502F91FD75D898E5C12394AEA92437EF61A392CA500BFBCB5680DA5B5BFE87B1A426FAF67154C94B06D42770AD646EEDE1F7D7771971FAAB9E9F84855D2627A09CC4AEDB86FDC175625635DE782D85CB7C702818109C306A6E8BA1363D7F2DB30C2F5492D9455C67F8698727A021C213D23EB904CCA2C256B3FD0037FB7978E4C7E2EDAA24439AED9454A0A167CBEBACB96A0FA27A9B006C205DB65B451A88BF20B3BF3D5C848D4CC860BEDCB978EA5F9797B67616F4F3DE3C565E0C548CE51F77293D0F8930148FCA4344368759E4CCA8B8236DFE10281810524686C26BA518E421E925FA107DB5FC859BB54F92A5392A1517D0A517221079F28C9562AAEB78B41939C8CB3C1B92A042617C420ED67AB84217689ABB65CC385B0C26F1A8AAE4515602386E1B82D20A1615B5BF15C42320D6C68205B304BC50C7CDC1CB181B32A9C670172FB1B97DC4FCF0D6BFF724EA28FD54380FEDF33B443028181014433F77CDABF486BA6B98A63028C92EFEC6B66C7B6A8E011F6EC1E63D264984250DD4D46A38702D51F6BDA27237D81E020776EC3FD24D6F42AFFD555D9AE010B8E1D7F2475C5A398EB49B992B518C0D4FE92856163EA4FCAA75CCC05028FAA55DE320ECE37D27030645282FEF63D869248121F15C828AB244A79F78D47598256"
   }
   
   stages {
      stage('Pull') {
         steps {
            
            // Get some code from a GitHub repository
            git branch: 'keystore_sidechain', credentialsId: 'brettchaldecott', url: 'git@github.com:brettchaldecott/keto.git'

            // Run Maven on a Unix agent.
            sh "git submodule update --init --recursive"
            
            // update submodules
            sh "cd docker && git checkout master"
            sh "cd docker && git pull"
            sh "cd src/contracts/keto_standard_typscript_contracts && git checkout master"
            sh "cd src/contracts/keto_standard_typscript_contracts && git pull"
            sh "cd src/web/avertem-restapi && git checkout master"
            sh "cd src/web/avertem-restapi && git pull"
            sh "cd src/web/keto-blockchain-explorer && git checkout master"
            sh "cd src/web/keto-blockchain-explorer && git pull"
            sh "cd src/web/keto-middlware-proxy && git checkout master"
            sh "cd src/web/keto-middlware-proxy && git pull"
            sh "cd src/web/keto-wallet && git checkout master"
            sh "cd src/web/keto-wallet && git pull"
        }
      }
            
      stage('Restore Keys') {
         steps {
            sh "if [ -d ~/keys/`cat ~/keys/latest`/resources/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/resources/* ./src/resources/keys/.; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/consensus_module/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/consensus_module/* ./src/modules/consensus_module/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/account_module/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/account_module/* ./src/modules/account_module/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/balancer_module/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/balancer_module/* ./src/modules/balancer_module/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/block_module/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/block_module/* ./src/modules/block_module/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/event_service/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/event_service/* ./src/modules/event_service/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/http_server/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/http_server/* ./src/modules/http_server/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/keystore/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/keystore/* ./src/modules/keystore/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/memory_vault_module/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/memory_vault_module/* ./src/modules/memory_vault_module/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/router_module/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/router_module/* ./src/modules/router_module/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/rpc_client/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/rpc_client/* ./src/modules/rpc_client/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/rpc_server/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/rpc_server/* ./src/modules/rpc_server/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/sandbox/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/sandbox/* ./src/modules/sandbox/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/test_module/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/test_module/* ./src/modules/test_module/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/transaction_service/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/transaction_service/* ./src/modules/transaction_service/keys/. ; fi"
            sh "if [ -d ~/keys/`cat ~/keys/latest`/version_manager/ ] ; then cp -rf ~/keys/`cat ~/keys/latest`/version_manager/* ./src/modules/version_manager/keys/. ; fi"
            
         }

      }
      stage('Pre-Build') {
         steps {
            // Run the build script
            sh "./manager.sh dev clean && docker system prune -f -a && ./manager.sh dev build"
         }

      }
      stage('Keys') {
         steps {
            // Run the build script
            
            sh "echo \"the key is now\""
            sh "cat ./src/resources/keys/key_1.json"
            sh "cat ./src/resources/keys/key_23.json"
            sh "./scripts/tools/GenerateKeys.sh 24 ./src/resources/keys 2"
            sh "echo \"the key should be different and not old faithful\""
            sh "cat ./src/resources/keys/key_1.json"
            sh "cat ./src/resources/keys/key_23.json"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/consensus_module/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/account_module/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/balancer_module/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/block_module/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/event_service/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/http_server/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/keystore/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/memory_vault_module/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/router_module/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/rpc_client/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/rpc_server/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/sandbox/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/test_module/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/transaction_service/keys 2"
            sh "./scripts/tools/GenerateKeysModuleKeys.sh ./src/resources/keys ./src/modules/version_manager/keys 2"
            sh "echo \"the key should have  been modified and should not be old faithful\""
            sh "cat ./src/resources/keys/key_1.json"
            sh "cat ./src/resources/keys/key_23.json"
            
            script {
                sh "./scripts/tools/KetoConsensusKeys.sh 24 ./src/resources/keys /tmp/SESSION_KEYS"
                sessionKey=readFile('/tmp/SESSION_KEYS').trim()
                sh "rm /tmp/SESSION_KEYS"
                sh "echo \"The session keys are SESSION_KEYS : ${sessionKey}\""
            }
         }

      }
      stage('Build') {
         steps {
            // Run the build script
            sh "./manager.sh dev build && ./manager.sh debian build ${env.VERSION_NUMBER}"
         }

      }
      stage('image') {
         steps {
            // Run the build script
            sh "./manager.sh genesis rebuild && ./manager.sh node build && docker tag avertem/avertem:latest avertem/avertem:${env.VERSION_NUMBER} && docker push avertem/avertem:${env.VERSION_NUMBER} && docker tag avertem/avertem:${env.VERSION_NUMBER} avertem/avertem:latest && docker push avertem/avertem:latest"
         }
      }
      stage('genesis docker image') {
         steps {
            // Run the build script
            sh "cd docker/genesis-build/ && cp ../../build/install/config/genesis.json . && docker build . -t avertem/genesis:${env.VERSION_NUMBER} && docker tag avertem/genesis:${env.VERSION_NUMBER} 10.128.0.8:31019/avertem/genesis:${env.VERSION_NUMBER} && docker push 10.128.0.8:31019/avertem/genesis:${env.VERSION_NUMBER}"
            catchError {
                sh """kubectl delete services avertem-config --namespace avertem"""
            }
            catchError {
                sh """kubectl delete deployments --namespace avertem avertem-config"""
            }
            sh """
            kubectl create deployment --image=10.128.0.8:31019/avertem/genesis:${env.VERSION_NUMBER} --namespace avertem avertem-config && \
            kubectl expose deployment avertem-config --port=80 --name=avertem-config --namespace avertem"""
         }
      }
      stage('master') {
          steps {
            // release a new version
            script {
                def yamlConfig = """
image:
  version: ${env.VERSION_NUMBER}
service:
  env:
    PRODUCER_ENABLED: "true"
    KETO_account_hash: "${env.MASTER_ACCOUNT_HASH}"
    KETO_public_key: "${env.MASTER_PUBLIC_KEY}"
    KETO_private_key: "${env.MASTER_PRIVATE_KEY}"
    KETO_consensus_keys: "${sessionKey}"
    KETO_IS_MASTER: "true"
    KETO_rpc_peer : "EMPTY"
    KETO_genesis_url : "http://avertem-config/genesis.json"

persistence:
  existingClaim: "avertem-master"    
""";
                writeFile file: "/tmp/master_config.yaml", text: yamlConfig
                catchError {
                    sh """cd ~/helm/avertem_helm/ && \\
                        helm uninstall avertem-master --namespace avertem"""
                }
                sh """cd ~/helm/avertem_helm/ && \\
                helm install avertem-master . --namespace avertem  -f /tmp/master_config.yaml"""
            }
          }
      }
      stage('Upload') {
         steps {
            // Run the build script
            sh "s3cmd put -P ${pwd()}/release/avertem_${env.VERSION_NUMBER}_all.deb s3://avertem/linux/ubuntu/18.04/${env.VERSION_NUMBER}/avertem_${env.VERSION_NUMBER}_all.deb"
            sh "s3cmd put -P ${pwd()}/release/avertem_shared_${env.VERSION_NUMBER}.tar.gz s3://avertem/linux/ubuntu/18.04/${env.VERSION_NUMBER}/avertem_shared_${env.VERSION_NUMBER}.tar.gz"
            sh "s3cmd put -P ${pwd()}/release/latest_version.txt s3://avertem/linux/ubuntu/18.04/latest_version.txt"
         }

      }
      stage('Backup Keys') {
        steps {
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/resources/ && cp -rf ./src/resources/keys/* ~/keys/${env.VERSION_NUMBER}/resources/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/consensus_module/ && cp -rf ./src/modules/consensus_module/keys/* ~/keys/${env.VERSION_NUMBER}/consensus_module/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/account_module/ && cp -rf ./src/modules/account_module/keys/* ~/keys/${env.VERSION_NUMBER}/account_module/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/balancer_module/ && cp -rf ./src/modules/balancer_module/keys/* ~/keys/${env.VERSION_NUMBER}/balancer_module/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/block_module/ && cp -rf ./src/modules/block_module/keys/* ~/keys/${env.VERSION_NUMBER}/block_module/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/event_service/ && cp -rf ./src/modules/event_service/keys/* ~/keys/${env.VERSION_NUMBER}/event_service/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/http_server/ && cp -rf ./src/modules/http_server/keys/* ~/keys/${env.VERSION_NUMBER}/http_server/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/keystore/ && cp -rf ./src/modules/keystore/keys/* ~/keys/${env.VERSION_NUMBER}/keystore/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/memory_vault_module/ && cp -rf ./src/modules/memory_vault_module/keys/* ~/keys/${env.VERSION_NUMBER}/memory_vault_module/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/router_module/ && cp -rf ./src/modules/router_module/keys/* ~/keys/${env.VERSION_NUMBER}/router_module/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/rpc_client/ && cp -rf ./src/modules/rpc_client/keys/* ~/keys/${env.VERSION_NUMBER}/rpc_client/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/rpc_server/ && cp -rf ./src/modules/rpc_server/keys/* ~/keys/${env.VERSION_NUMBER}/rpc_server/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/sandbox/ && cp -rf ./src/modules/sandbox/keys/* ~/keys/${env.VERSION_NUMBER}/sandbox/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/test_module/ && cp -rf ./src/modules/test_module/keys/* ~/keys/${env.VERSION_NUMBER}/test_module/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/transaction_service/ && cp -rf ./src/modules/transaction_service/keys/* ~/keys/${env.VERSION_NUMBER}/transaction_service/."
            sh "mkdir -p ~/keys/${env.VERSION_NUMBER}/version_manager/ && cp -rf ./src/modules/version_manager/keys/* ~/keys/${env.VERSION_NUMBER}/version_manager/."
            sh "echo '${env.VERSION_NUMBER}' > ~/keys/latest"
        }
      }
      
   }
}
