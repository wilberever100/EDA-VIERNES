// Copyright 2020 Roger Peralta Aranibar
#ifndef SOURCE_PARTIALDIRECTEDGRAPH_HPP_
#define SOURCE_PARTIALDIRECTEDGRAPH_HPP_

#include <utility>

#include "DirectedGraph.hpp"
using namespace std;
namespace ADE {
namespace Persistence {

template <typename TipoNodo, typename TDato>
class modi {
 public:
  TipoNodo* ptr;  //
  // TipoNodo** doublePointer;
  TDato value;
  unsigned int version;
  size_t index;
  bool puntero;
  modi(TDato val, int ver)
      : value(val),
        version(ver),
        puntero(false),
        ptr(nullptr) {}  // modifica valor del mismo nodo
  modi(size_t ind, TipoNodo* ptr_, unsigned int ver)  // ptr al nodo modificado
      : ptr( ptr_),
        //ptr(dynamic_cast < PartialNode<TDato> * (ptr_)),
        version(ver),
        puntero(true),
        //value(null),
        index(ind) {}  // modifica un forward
};
template <typename Type>
class PartialNode : public Node<Type> {
 public:
  typedef Type data_type;
  std::vector<modi<PartialNode, data_type>*> mods;  // vector de modificaciones
  int version_;
  int usedMods;              // modificaciones usadas hasta 2*in_ptrs
  std::vector<PartialNode*> back_;  // back pointers
  std::size_t in_ptrs_size_;


  PartialNode(data_type data, std::size_t const& out_ptrs_size)
      : Node<Type>(data, out_ptrs_size),
        usedMods(0) {}


  
  data_type get_data() { return this->data_; }
  data_type get_data(unsigned int version) {
    data_type dato;
    for (auto e : mods) {
      if (e->version <= version) {
        if (!e->puntero) {  // si es que no es un puntero cambiado sino un dato
          dato = e->value;
        }
      } else {
        break;
      }
    }
    return dato;
    // return *Node<Type>::data_;
  }
  
  PartialNode* verify_mods(unsigned int version) {  // en caso pase 2p crea un nuevo nodo
                                // representando al mismo con 0 mods
    if (usedMods > 2 * in_ptrs_size_) {
      PartialNode<Type>* new_nodo =
          new PartialNode<Type>(*(this->data_), this->out_ptrs_size_, this->in_ptrs_size_);

      //new_nodo->forward_ = this->forward_;
      new_nodo->back_ = this->back_;
      
      for (auto m : mods) {
      //for (auto m = mods.begin(); m != mods.end(); m++) {
          if ((*m)->puntero == false) {   //modifica el dato
            *(new_nodo->data_)=(*m)->value;
          } else {
            new_nodo->forward_[(*m)->index] = (*m)->ptr;    //modifica y actualiza sus fordward
          }

      }
      //el nuevo nodo a este punto tiene sus datos correctos
      for (int i = 0; i < this->out_ptrs_size_; i++) {
        // for (auto f : new_nodo->forward_) {
        for (auto e : *forward_[i]->back_) {
          if (*e == this) {
            *e = new_nodo;  // actualizamos backpointer de los nodos forward de
                            // este nodo
          }
        }
      }
      int k = 0;
      for (auto b : new_nodo->back_) {
        k = 0;
        for (auto f : *b->forward_) {
          
          if (*f == this) {
            
            *f = new_nodo;      //actualizamos los forward de los que nos apuntan
            if (!*f->verify_mods(version)) {         //si hay espacio en modificaciones de esos nodos que me apuntan
                                                     //sino será recursivo y creando nuevos nodos
              *f->mods.push_back(new modi<PartialNode,data_type>(k, new_nodo, version)); 
              //Colocamos mods en los nodos que apuntan a este para que apunten al nuevo
            } 
                           

          }
          k++;
        }
      }


      return new_nodo;
    } else {
      return nullptr;   //Hay espacio een mods
    }
  }

 

  bool set_data(data_type const data, unsigned int version) {
    ++usedMods;
    PartialNode* ptr;
    if (!(ptr = verify_mods(version)))  // en caso sea nulo, es decir que tenga espacio
                               // para mods inserta aquí
      this->mods.push_back(new modi<PartialNode,data_type>(data, version));
    else {
      *(ptr->data_)=data; //simplemente le modifica el valor porque es uno nuevo
    }
    //*Node<Type>::data_ = data;
    return true;
  }

  bool set_ptr(PartialNode* ptr, unsigned int id) {
    Node<Type>::forward_ = ptr;
    return true;
  }
  PartialNode& operator[](std::size_t id) const {
    return *dynamic_cast<PartialNode*>(&(Node<Type>::operator[](id)));
  }
  PartialNode& operator[](
      std::pair<std::size_t, unsigned int> id_version) const {
    return *dynamic_cast<PartialNode*>(
        &(Node<Type>::operator[](id_version.first)));
  }
};

template <typename Type, typename Node = PartialNode<Type>>
class PartialDirectedGraph : public DirectedGraph<Type, Node> {
 public:
  typedef Type data_type;

  PartialDirectedGraph(data_type const data, std::size_t const& out_ptrs_size,
                       std::size_t const& in_ptrs_size)
      : DirectedGraph<Type, Node>(data, out_ptrs_size),
        in_ptrs_size_(in_ptrs_size),
        current_version(0) {
    Node* p = this->get_root_ptr(0);
    p->in_ptrs_size_ = this->in_ptrs_size_;
    p->version_ = 0;
  }

  Node* get_root_ptr(unsigned int version) {
    return DirectedGraph<Type, Node>::get_root_ptr();
  }

  Node* insert_vertex(data_type const data, Node* u, std::size_t position) {
    ++current_version;
    Node* p = insert_vertex(data, u, position, current_version);    
    p->in_ptrs_size_ = this->in_ptrs_size_;     //coloca al vertice ingresado in_ptr
    p->version_ = current_version;
    return p;
  }

  void add_edge(Node* u, Node* v, std::size_t position) {
    ++current_version;
    return add_edge(u, v, position, current_version);
  }

 private:
  Node* insert_vertex(data_type const data, Node* u, std::size_t position,
                      unsigned int version) {
    Node* next = dynamic_cast<Node*>(u->forward_[position]);

    Node* p = dynamic_cast<Node*>(
        DirectedGraph<Type, Node>::insert_vertex(data, u, position));

    p->back_.push_back(u);
    /*if (next) {
      next->back_.push_back(p);
      for (auto n : next->back_) {          //buscamos el u[pos]=next para borrarle su backpointer a u
        if (*n!=nullptr) {
          if (*n == u) {
            next->back_.erase(n);
          }
        }
      }
    }
    */
    return p;
  }

  void add_edge(Node* u, Node* v, std::size_t position, unsigned int version) {
    //u->forward_[position] = v;  // agregamos el edge
    Node* ptr;
    if (!(ptr=u->verify_mods(version))) {

      modi<Node, data_type>* nuevo =new modi<Node, data_type>(position, u, version);
      u->mods.push_back(nuevo);  // agregamos la mod a u
                                                          
    }
    else {
      ptr->forward_[position] = v;  // agregamos el edge en el nuevo nodo creado
    }
    if (v) {
      v->back_.push_back(u);    //agregamos a v el backpointer  u
    }

    return DirectedGraph<Type, Node>::add_edge(u, v, position);
  }

  std::size_t in_ptrs_size_;
  unsigned int current_version;
};

}  // namespace Persistence
}  // namespace ADE

#endif  // SOURCE_PARTIALDIRECTEDGRAPH_HPP_