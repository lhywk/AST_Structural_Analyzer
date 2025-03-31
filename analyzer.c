#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "json_c.c"

// JSON AST에서 If 조건문을 찾아 개수 세는 재귀 함수
int count_if(json_value node) {
    int count = 0;

    // 객체 타입인 경우
    if (node.type == JSON_OBJECT) {
        // 현재 노드의 _nodetype 확인
        json_value nodetype = json_get(node, "_nodetype");
        // If인 경우 count 증가
        if (strcmp(json_to_string(nodetype), "If") == 0) 
            count++;

        // 객체 내부의 모든 값에 대해 재귀 호출
        int len = json_get_last_index(node);
        json_object* obj = (json_object*)node.value;
        for (int i = 0; i <= len; i++) {
            count = count + count_if(obj->values[i]);
        }

    } else if (node.type == JSON_ARRAY) {
        // 배열인 경우 각 원소에 대해 재귀 호출
        for (int i = 0; i < json_len(node); i++) {
            count = count + count_if(json_get(node, i));
        }
    }

    return count;
}

// 함수 정의 목록 순회, 이름/리턴타입/파라미터/if문 출력
void extraction(json_value root) {
    int function_count = 0;

    // 루트 배열의 각 노드 순회
    for (int i = 0; i < json_len(root); i++) {
        json_value node = json_get(root, i);

        // FuncDef 노드만 필터링 (실제 함수 정의)
        json_value nodetype = json_get(node, "_nodetype");
        if (strcmp(json_to_string(nodetype), "FuncDef") != 0)
            continue;

        function_count++;

        // 함수 이름 출력
        json_value decl = json_get(node, "decl");
        json_value name = json_get(decl, "name");
        printf("Function #%d: %s\n", function_count, json_to_string(name));

        // 리턴 타입 분석
        json_value func_type = json_get(decl, "type");        // FuncDecl
        json_value return_type = json_get(func_type, "type"); // TypeDecl 또는 PtrDecl

        int is_pointer = 0;
        // 포인터인지 검사
        json_value ptr_check = json_get(return_type, "_nodetype");
        if (strcmp(json_to_string(ptr_check), "PtrDecl") == 0) {
            is_pointer = 1;
            return_type = json_get(return_type, "type"); // 내부의 TypeDecl로 이동
        }

        printf("  Return type: ");

        // 최종 타입 이름 추출 int, char 등등
        json_value typedecl_type = json_get(return_type, "type");
        json_value return_names = json_get(typedecl_type, "names");

        for (int j = 0; j < json_len(return_names); j++) {
            if (is_pointer)
                printf("%s* ", json_to_string(json_get(return_names, j)));
            else 
                printf("%s ", json_to_string(json_get(return_names, j)));
        }
        printf("\n");

        // 파라미터 분석
        json_value args = json_get(func_type, "args");
        if (args.type != JSON_OBJECT) {
            printf("  Parameters: 없음\n");
        }
        else {
            json_value params = json_get(args, "params");

            printf("  Parameters: \n");
            for (int k = 0; k < json_len(params); k++) {
                json_value param = json_get(params, k);
                json_value ptype = json_get(param, "type");

                // 파라미터가 포인터인지 확인
                int param_is_pointer = 0;
                json_value ptr = json_get(ptype, "_nodetype");
                if (strcmp(json_to_string(ptr), "PtrDecl") == 0) {
                    param_is_pointer = 1;
                    ptype = json_get(ptype, "type"); // 내부 TypeDecl로
                }

                json_value real_type = json_get(ptype, "type");

                printf("    - ");
                

                // 타입 이름 출력
                json_value names = json_get(real_type, "names");
                for (int j = 0; j < json_len(names); j++) {
                    if (param_is_pointer)
                        printf("%s* ", json_to_string(json_get(names, j)));
                    else 
                        printf("%s ", json_to_string(json_get(names, j)));
                }
                // 변수 이름 출력
                json_value pname = json_get(param, "name");
                printf("%s\n", json_to_string(pname));
            } 
        }
         //  함수 내 if 조건문 개수 출력
        json_value body = json_get(node, "body");
        int if_count = count_if(body);
        printf("  If 조건문 수: %d개\n", if_count);
        printf("\n");
    }

    printf("총 함수 수: %d\n", function_count);
}

int main() {
    // AST JSON 파일 열기
    FILE *fp = fopen("C/ast.json", "r");

    // 파일 크기 측정 및 읽기
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);

    // 파일 내용을 버퍼에 읽어오기
    char *buffer = malloc(length + 1);
    fread(buffer, 1, length, fp);
    buffer[length] = '\0';
    fclose(fp);

    // 문자열을 파싱하여 JSON 구조 생성
    json_value root = json_create(buffer);
    free(buffer);

    // 상위 노드인 ext 배열 추출
    json_value ext = json_get(root, "ext");

    // 함수 분석 수행
    extraction(ext);

    // 메모리 정리
    json_free(root);
    return 0;
}
