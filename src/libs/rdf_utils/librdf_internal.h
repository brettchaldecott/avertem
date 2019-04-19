//
// Created by Brett Chaldecott on 2019/04/17.
//

#ifndef KETO_LIBRDF_INTERNAL_H
#define KETO_LIBRDF_INTERNAL_H

#include <raptor2.h>

/**
 * rasqal_projection:
 * @query: rasqal query
 * @wildcard: non-0 if @variables was '*'
 * @distinct: 1 if distinct, 2 if reduced, otherwise neither
 *
 * Query projection (SELECT vars, SELECT *, SELECT DISTINCT/REDUCED ...)
 *
 */
typedef struct {
    rasqal_query* query;

    raptor_sequence* variables;

    unsigned int wildcard:1;

    int distinct;
} rasqal_projection;


/**
 * rasqal_solution_modifier:
 * @query: rasqal query
 * @order_conditions: sequence of order condition expressions (or NULL)
 * @group_conditions: sequence of group by condition expressions (or NULL)
 * @having_conditions: sequence of (group by ...) having condition expressions (or NULL)
 * @limit: result limit LIMIT (>=0) or <0 if not given
 * @offset: result offset OFFSET (>=0) or <0 if not given
 *
 * Query solution modifiers
 *
 */
typedef struct {
    rasqal_query* query;

    raptor_sequence* order_conditions;

    raptor_sequence* group_conditions;

    raptor_sequence* having_conditions;

    int limit;

    int offset;
} rasqal_solution_modifier;


typedef struct {
    /* usage/reference count */
    int usage;

    rasqal_query* query;

    raptor_sequence* variables;

    raptor_sequence* rows;
} rasqal_bindings;


/*
 * Graph Pattern
 */
struct rasqal_graph_pattern_s {
    rasqal_query* query;

    /* operator for this graph pattern's contents */
    rasqal_graph_pattern_operator op;

    raptor_sequence* triples;          /* ... rasqal_triple*         */
    raptor_sequence* graph_patterns;   /* ... rasqal_graph_pattern*  */

    int start_column;
    int end_column;

    /* the FILTER / LET expression */
    rasqal_expression* filter_expression;

    /* index of the graph pattern in the query (0.. query->graph_pattern_count-1) */
    int gp_index;

    /* Graph literal / SERVICE literal */
    rasqal_literal *origin;

    /* Variable for LET graph pattern */
    rasqal_variable *var;

    /* SELECT projection */
    rasqal_projection* projection;

    /* SELECT modifiers */
    rasqal_solution_modifier* modifier;

    /* SILENT flag for SERVICE graph pattern */
    unsigned int silent : 1;

    /* SELECT graph pattern: sequence of #rasqal_data_graph */
    raptor_sequence* data_graphs;

    /* VALUES bindings for VALUES and sub-SELECT graph patterns */
    rasqal_bindings* bindings;
};

#endif //KETO_LIBRDF_INTERNAL_H
