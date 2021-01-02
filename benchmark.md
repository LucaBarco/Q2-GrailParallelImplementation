# In memory version

## Small dense

|        File        | #Nodes | #Labels | #Queries | Graph Reading Time | Creation Labels Time | Query resolving time | AVG query resolving time | Memory used |
|--------------------|--------|---------|----------|--------------------|----------------------|----------------------|--------------------------|-------------|
| arXiv_sub_6000-1   | 6000   |   3     |   12000  |   0.556s           |     0.173s           |   3.469s             |       0.000012s          |   7.5MB     |
| citeseer_sub_10720 | 10720  |   3     |   22000  |   0.375s           |     0.282s           |   5.398s             |       0.000003s          |   6.2MB     |
| go_sub_6793        | 6793   |   3     |   15000  |   0.126s           |     0.193s           |   3.912s             |       0.000002s          |   3.2MB     |
| pubmed_sub_9000_1  | 9000   |   3     |   18000  |   0.341s           |     0.249s           |   4.435s             |       0.000002s          |   5.7MB     |
| yago_sub_6642      | 6642   |   3     |   12000  |   0.376s           |     0.191s           |   3.245s             |       0.000002s          |   5.6MB     |

## Small sparse

|        File        | #Nodes | #Labels | #Queries | Graph Reading Time | Creation Labels Time | Query resolving time | AVG query resolving time | Memory used |
|--------------------|--------|---------|----------|--------------------|----------------------|----------------------|--------------------------|-------------|
| agrocyc_dag_uniq   | 12684  |   3     |  24000   |   0.176s           |     0.289s           |   5.993s             |       0.000002s          |    4.1MB    |
| amaze_dag_uniq     |  3710  |   3     |   8000   |   0.074s           |     0.122s           |   2.106s             |       0.000018s          |    2.0MB    |
| anthra_dag_uniq    | 12499  |   3     |  24000   |   0.179s           |     0.271s           |   5.995s             |       0.000368s          |    4.0MB    |
| ecoo_dag_uniq      | 12620  |   3     |  24000   |   0.172s           |     0.270s           |   6.024s             |       0.000352s          |    4.1MB    |
| human_dag_uniq     | 38811  |   3     |  60000   |   1.971s           |     2.093s           |  14.705s             |       0.000047s          |    9.9MB    |
| kegg_dag_uniq      |  3617  |   3     |   7000   |   0.069s           |     0.127s           |   1.770s             |       0.000020s          |    2.0MB    |
| mtbrv_dag_uniq     |  9602  |   3     |  18000   |   0.133s           |     0.241s           |   4.431s             |       0.000005s          |    3.2MB    |
| nasa_dag_uniq      |  5605  |   3     |  12000   |   0.079s           |     0.163s           |   3.029s             |       0.000002s          |    2.6MB    |
| vchocyc_dag_uniq   |  9491  |   3     |  20000   |   0.149s           |     0.235s           |   4.864s             |       0.000004s          |    3.4MB    |
| xmark_dag_uniq     |  6080  |   3     |  12000   |   0.087s           |     0.186s           |   3.310s             |       0.000003s          |    2.6MB    |

## Large

|        File        | #Nodes | #Labels | #Queries | Graph Reading Time | Creation Labels Time | Query resolving time | AVG query resolving time | Memory used |
|--------------------|--------|---------|----------|--------------------|----------------------|----------------------|--------------------------|-------------|
| uniprotenc_22m.scc | 1595444|   3     | 1000000  |    12.289s         |     40.574s          |      290.426s        |      0.000034s           |    375MB    |
| cit-Patents.scc    | 3774768|   3     | 3000000  |   139.592s         |     96.962s          |      852.691s        |      0.000081s           |    1.7GB    |


# File based version

## Small dense

|        File        | #Nodes | #Labels | #Queries | Graph Reading Time | Creation Labels Time | Query resolving time | AVG query resolving time | Memory used |
|--------------------|--------|---------|----------|--------------------|----------------------|----------------------|--------------------------|-------------|
| arXiv_sub_6000-1   | 6000   |   3     |   12000  |   10.107s          |     8.600s           |    297.471s          |      0.020249s           |    2.2MB    |
| citeseer_sub_10720 | 10720  |   3     |   22000  |   54.362s          |     6.090s           |     23.689s          |      0.000571s           |    3.6MB    |
| go_sub_6793        | 6793   |   3     |   15000  |    2.177s          |     2.267s           |     16.439s          |      0.000002s           |    2.3MB    |
| pubmed_sub_9000_1  | 9000   |   3     |   18000  |    6.355s          |     5.780s           |     19.517s          |      0.000558s           |    3.0MB    |
| yago_sub_6642      | 6642   |   3     |   12000  |    6.586s          |     5.846s           |     14.458s          |      0.000899s           |    2.2MB    |

## Small sparse

|        File        | #Nodes | #Labels | #Queries | Graph Reading Time | Creation Labels Time | Query resolving time | AVG query resolving time | Memory used |
|--------------------|--------|---------|----------|--------------------|----------------------|----------------------|--------------------------|-------------|
| agrocyc_dag_uniq   | 12684  |   3     |  24000   |   2.406s           |     3.857s           |   24.392s            |       0.000274s          |     4.3MB   |
| amaze_dag_uniq     |  3710  |   3     |   8000   |   0.711s           |     1.024s           |   33.833s            |       0.008342s          |     1.8MB   |
| anthra_dag_uniq    | 12499  |   3     |  24000   |   2.155s           |     3.751s           |   25.698s            |       0.000315s          |     4.1MB   |
| ecoo_dag_uniq      | 12620  |   3     |  24000   |   2.426s           |     4.017s           |   25.135s            |       0.000344s          |     3.9MB   |
| human_dag_uniq     | 38811  |   3     |  60000   |   6.786s           |    39.307s           |   70.200s            |       0.000731s          |    10.5MB   |
| kegg_dag_uniq      |  3617  |   3     |   7000   |   0.544s           |     0.945s           |   41.670s            |       0.011522s          |     1.8MB   |
| mtbrv_dag_uniq     |  9602  |   3     |  18000   |   1.713s           |     2.632s           |   18.815s            |       0.000347s          |     3.4MB   |
| nasa_dag_uniq      |  5605  |   3     |  12000   |   1.152s           |     1.302s           |   12.200s            |       0.000363s          |     2.2MB   |
| vchocyc_dag_uniq   |  9491  |   3     |  20000   |   1.643s           |     2.669s           |   20.551s            |       0.000338s          |     3.3MB   |
| xmark_dag_uniq     |  6080  |   3     |  12000   |   1.235s           |     1.455s           |   12.676             |       0.000489s          |     2.1MB   |

## Large

|        File        | #Nodes | #Labels | #Queries | Graph Reading Time | Creation Labels Time | Query resolving time | AVG query resolving time | Memory used |
|--------------------|--------|---------|----------|--------------------|----------------------|----------------------|--------------------------|-------------|
| uniprotenc_22m.scc | 1595444|   3     | 1000000  |      247.266s      |      367.259s        |    940.383s          |      0.001058s           |    385.7MB  |
| cit-Patents.scc    | 3774768|   3     | 3000000  |     3325.133s      |     3292.448s        |   2131.500s          |      0.001800s           |    854.0MB  |