/**
 * Copy right YMSys, 2015, Zhaoming Yin
 *
 * @brief    This is the class for CSR formatted graph. 
 *
 *  MODIFIED   (MM/DD/YY)
 *  stplaydog   12/13/15 - Fixed some bugs 
 *  stplaydog   12/10/15 - Add constructor
 *  stplaydog   12/10/15 - Fix build bugs
 *  stplaydog   12/08/15 - Implemented compute support 
 *  stplaydog   09/03/15 - Creation
 *
**/

#pragma once
#ifndef _H_CSR
#define _H_CSR

#include "utils.h"
#include <algorithm>

using namespace std; 

/**
 * @class CSR 
 *
 * This class defines a CSR, compressed sparse row; 
 * and provides basic operations on CSR.
 *
**/
class CSR
{

public:

    enum DEF
    {
        RMVD = -1
    };

    /**
     * @brief   This is the constructor
     *
    **/
    CSR(const char *in_file)
    {
        int32_t v_num = OPTKIT_NULL;
        int32_t e_num = OPTKIT_NULL;
        int32_t c_num = OPTKIT_NULL;
        int32_t v_id  = OPTKIT_NULL;
        int32_t v_to  = OPTKIT_NULL;
        int32_t color = OPTKIT_NULL;
        int32_t sum   = 0;

        /* Read the head information, 
           and allocate according variables */
        FILE *reader;
        if((reader = fopen(in_file, "r")) == NULL)
        {
            printf("the file %s you input does not exist!\n", in_file);
            ERROR_PRINT();
        }
        else if(fscanf(reader, "%d %d %d\n", &v_num, &c_num, &e_num)==EOF)
        {
            ERROR_PRINT();
        }

        int32_t start_read_pos = ftell(reader);

        allocate_data_structure(v_num, e_num, c_num);

        /* Scan the real content */
        for(int32_t i=0;i<e_num;i++)
        {
            if(fscanf(reader, "%d %d %d\n",&v_id , &v_to, &color)==EOF)
            {
                ERROR_PRINT();
            }
            ++v_idx[color][v_id];
        }

        for(int32_t c=0; c<num_c; c++)
        {
            sum = 0;
            for(int32_t i=0; i<v_num; i++)
            {
                sum += v_idx[c][i];
                v_idx[c][i]=sum;
            }
        }

        /* Populate the content */
        fseek (reader, start_read_pos, SEEK_SET);

        /* this is for the start of the different postitions */
        int32_t *idx = new int32_t[num_v+1];

        for(int32_t c=0; c<num_c; c++)
        {
            for(int32_t i=0; i<num_v; i++)
            {
                idx[i] = i == 0 ? 0 : v_idx[c][i-1];
            }

            /* Real read part */
            for(int32_t i=0; i<num_e; i++)
            {
                if(fscanf(reader, "%d %d %d\n",&v_id , &v_to, &color)==EOF)
                {
                    ERROR_PRINT();
                }

                int32_t pos = idx[v_id];
                e_idx[c][pos] = v_to;
                ++idx[v_id];
            }

            /* e_idx also needs to be sorted, for the purpose of join operations */
            for(int32_t i=0; i<num_v; i++)
            {
                pair<int32_t, int32_t> rg = get_e_range(i);
                sort(e_idx[c]+rg.first, e_idx[c]+rg.second);
            }
        }

        /* Frees */
        delete [] idx;
        fclose(reader);
    }

    /**
     * @brief   Destrutor
     *
    **/
    ~CSR()
    {
        for(int c=0; c<num_c; c++)
        {
            delete [] v_idx[c];
            delete [] e_idx[c];
        }

        delete [] v_idx;
        delete [] e_idx;
    }

    /**
     * Allocate memory for basic graph data structures. 
     *
     * @param[in]       v_num       number of vertices
     * @param[in]       e_num       number of edges 
     *
     * @return      N/A
     **/
    void allocate_data_structure(const int32_t v_num, 
            const int32_t e_num, const int32_t c_num)
    {
        num_v = v_num;
        num_e = e_num;
        num_c = c_num;

        v_idx = new int32_t*[num_c];
        e_idx = new int32_t*[num_c];

        for(int32_t c=0; c<num_c; c++)
        {
            v_idx[c] = new int32_t[num_v];
            e_idx[c] = new int32_t[num_e];
            memset(v_idx[c], 0, sizeof v_idx[c] * sizeof(int));
            memset(e_idx[c], 0, sizeof e_idx[c] * sizeof(int));
        }
    }

    /**
     *
     * @note    edge list has to be sorted.
    **/
    int32_t compute_num_edge_intersect(pair<int32_t, int32_t> rg1,
                                       pair<int32_t, int32_t> rg2,
                                       int32_t c = 0)
    {
        int32_t ret = 0;
        int32_t i(rg1.first), j(rg2.first);
        while(i<rg1.second && j<rg2.second)
        {
            if(e_idx[c][i] == e_idx[c][j])
            {
                ++ret;
                ++i;
                ++j;
            }
            else if(e_idx[c][i] > e_idx[c][j])
            {
                ++j;
            }
            else
            {
                ++i;
            }
        }
        return ret;
    }

    
    /**
     * @return  number of vertices 
    **/
    int32_t get_num_v()
    {
        return num_v;
    }

    /**
     * @return  number of edges 
    **/
    int32_t get_num_e()
    {
        return num_e;
    }

    /**
     * @return  number of colors 
    **/
    int32_t get_num_c()
    {
        return num_c;
    }

    int32_t get_to_v(int32_t pos, int32_t c = 0)
    {
        return e_idx[c][pos];
    }

    void set_rmvd(int32_t pos, int32_t c = 0)
    {
        e_idx[c][pos] = RMVD;
    }

    /**
     * @brief   Reconstruct the graph, because some edges has been removed.
     *
     * @return  N/A
    **/
    void reconstruct(int32_t c = 0)
    {
        assert(c < num_c);

        /* update v_idx */
        int32_t *prefix_sum = new int32_t[num_v];
        memset(prefix_sum, 0, num_v * sizeof(int32_t));
        int32_t sum = 0; 

        for(int32_t i=0; i<num_v; i++)
        {
            int32_t count = 0;

            pair<int32_t, int32_t> rg = get_e_range(i, c);
            for(int32_t j=rg.first; j<rg.second; ++j)
            {
                if(e_idx[c][j] != RMVD)
                {
                    ++count;
                }
            }

            sum += count;

            prefix_sum[i] = sum;
        }

        memcpy(v_idx[c], prefix_sum, num_v * sizeof(int32_t));

        delete [] prefix_sum;

        /* moving elements in e_idx */
        int32_t cur=0, nxt=0;
        while(nxt < num_e)
        {
            if(e_idx[c][nxt] == RMVD) 
            {
                ++nxt;
            }
            else
            {
                e_idx[c][cur] = e_idx[c][nxt];
                ++cur;
                ++nxt;
            }
        }
    }

    /**
     * @brief       Remove one edge
     *
     * @param[in]       from        from vertex
     * @param[in]       to          to vertex
     * @param[in]       c           which color
     *
     * @return      Ture if there is such an edge to be removed
     *              else return false.
    **/
    bool remove_e(int32_t from, int32_t to, int c = 0)
    {
        int8_t find = 0;
        pair<int32_t, int32_t> rg_from = get_e_range(from, c); 
        pair<int32_t, int32_t> rg_to   = get_e_range(to, c); 

        for(int32_t i=rg_from.first; i<rg_from.second; ++i)
        {
            if(e_idx[c][i] == to)
            {
                e_idx[c][i] = RMVD;
                ++find;
            }
        }

        for(int32_t i=rg_to.first; i<rg_to.second; ++i)
        {
            if(e_idx[c][i] == from)
            {
                e_idx[c][i] = RMVD;
                ++find;
            }
        }

        if(find == 2)
            return true;
        return false;
    }

    /**
     * @brief   output the truss as connected components
     *
     * @param[in]       writer      the file which is going to be written
     * @param[in]       c           which color
     *
     * @return      N/A
     **/
    void output_all_CC(FILE *writer, int32_t c = 0)
    {
        assert(c < num_c);
        assert(writer  != NULL);

        bool *visited = new bool[num_v];

        int32_t count = 0;
        for(int32_t i=0; i<num_v; i++)
        {
            if(!visited[i])
            {
                fprintf(writer, "Comp [%d] :", count++);
                output_one_CC(writer, i, visited, c);
                fprintf(writer, "\n");
            }
        }

        delete [] visited;
    }
    
    /**
     * @brief   get the range of e_idxs from a given vertex
     *
     * @param[in]       v           given vertex
     * @param[in]       c           which color
     *
     * @return      the range
     *
    **/
    pair<int32_t, int32_t> get_e_range(int32_t v, int32_t c = 0)
    {
        pair<int32_t, int32_t> ret;
        ret.first  = v == 0 ? 0 : v_idx[c][v-1];
        ret.second = v_idx[c][v];
        return ret;
    }

    /**
     * @brief   visualize CSR using graphviz format
     *
    **/
    void visualize()
    {
        FILE *writer = fopen("./csr.dot", "w");
        fprintf(writer, "graph{\n");
        for(int c=0; c<num_c; c++)
        {
            char *color;
            if(c==0)
            {
                color = (char*)"red";
            }
            if(c==1)
            {
                color = (char*)"blue";
            }
            if(c==2)
            {
                color = (char*)"green";
            }
            for(int i=0; i<num_v; i++)
            {
                pair<int32_t, int32_t> rg = get_e_range(i);
                for(int j=rg.first; j<rg.second; j++)
                {
                    fprintf(writer, "%d -- %d [color=%s];\n", i, e_idx[c][j], color);
                }
            }
        }
        fprintf(writer, "}\n");
        fclose(writer);
    }

private:

    int32_t num_v;   ///< total number of vertices 
    int32_t num_e;   ///< total number of edges
    int32_t num_c;   ///< number of colors 
                     ///< (different edges have differen colors)
    int32_t **v_idx; ///< vertex index for CSR
    int32_t **e_idx; ///< edge index for CSR

    /**
     * @recursive algorithm to output a specific CC.
     *
     * @param[in]       writer      the file which is going to be written
     * @param[in]       v           the seed vertex 
     * @param[in]       visited     indicate which vertex has been visited
     *
     * @return      N/A
     **/
    void output_one_CC(FILE *writer, int32_t v, bool *visited, int32_t c = 0)
    {
        assert(c < num_c);
        assert(writer  != NULL);
        assert(visited != NULL);
        assert(v > 0 && v < num_v);

        if(!visited[v])
        {
            fprintf(writer, "%d ", v);
            visited[v] = true;
        }
        else
        {
            return;
        }

        pair<int32_t, int32_t> rg = get_e_range(v);
        for(int32_t i=rg.first; i<rg.second; ++i)
        {
            output_one_CC(writer, e_idx[c][i], visited);
        }
    }
};

#endif
