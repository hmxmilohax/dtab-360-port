#include "DTASerializer.h"

void serializeDTA(const DTA& dta, std::ostream& stream) {
    stream.write((const char*) &dta.byteZero, sizeof(dta.byteZero));
    serializeTree(dta.topTree, stream);
}

DTA deserializeDTA(std::istream& stream) {
    DTA dta;
    stream.read((char*) &dta.byteZero, sizeof(dta.byteZero));
    dta.topTree = deserializeTree(stream);
    return dta;
}

void serializeTree(const Tree& tree, std::ostream& stream) {
    stream.write((const char*) &tree.nodeID, sizeof(tree.nodeID));
    serializeChunks(tree.chunks, stream);
}

Tree deserializeTree(std::istream& stream) {
    Tree tree;
    stream.read((char*) &tree.nodeID, sizeof(tree.nodeID));
    tree.chunks = deserializeChunks(stream);
    return tree;
}

void serializeChunks(const std::vector<Chunk>& chunks, std::ostream& stream) {
    for (const Chunk& chunk : chunks) {
        stream.write((const char*) &chunk.tag, sizeof(chunk.tag));
        switch (chunk.tag) {
            case Chunk::Tag::INT:
                stream.write((const char*) &chunk.i, sizeof(chunk.i));
                break;
            case Chunk::Tag::FLOAT:
                stream.write((const char*) &chunk.f, sizeof(chunk.f));
                break;
            case Chunk::Tag::VAR:
            case Chunk::Tag::SYM:
            case Chunk::Tag::IFDEF:
            case Chunk::Tag::STRING:
            case Chunk::Tag::DEFINE:
            case Chunk::Tag::INCLUDE:
            case Chunk::Tag::MERGE:
            case Chunk::Tag::IFNDEF:
            case Chunk::Tag::UNDEF:
                stream.write((const char*) &chunk.str.size, sizeof(chunk.str.size));
                stream.write(chunk.str.data, chunk.str.size);
                break;
            case Chunk::Tag::PARENS:
            case Chunk::Tag::BRACES:
            case Chunk::Tag::BRACKETS:
                serializeTree(chunk.subtree, stream);
                break;
            default:
                break;
        }
    }
}

std::vector<Chunk> deserializeChunks(std::istream& stream) {
    std::vector<Chunk> chunks;
    while (stream.peek() != EOF) {
        Chunk chunk = deserializeChunk(stream);
        chunks.push_back(chunk);
    }
    return chunks;
}

Chunk deserializeChunk(std::istream& stream) {
    Chunk chunk;
    stream.read((char*) &chunk.tag, sizeof(chunk.tag));
    switch (chunk.tag) {
        case Chunk::Tag::INT:
            stream.read((char*) &chunk.i, sizeof(chunk.i));
            break;
        case Chunk::Tag::FLOAT:
            stream.read((char*) &chunk.f, sizeof(chunk.f));
            break;
        case Chunk::Tag::VAR:
        case Chunk::Tag::SYM:
        case Chunk::Tag::IFDEF:
        case Chunk::Tag::STRING:
        case Chunk::Tag::DEFINE:
        case Chunk::Tag::INCLUDE:
        case Chunk::Tag::MERGE:
        case Chunk::Tag::IFNDEF:
        case Chunk::Tag::UNDEF:
            stream.read((char*) &chunk.str.size, sizeof(chunk.str.size));
            chunk.str.data = new char[chunk.str.size];
			stream.read(chunk.str.data, chunk.str.size);
            break;
        case Chunk::Tag::PARENS:
        case Chunk::Tag::BRACES:
        case Chunk::Tag::BRACKETS:
            chunk.subtree = deserializeTree(stream);
            break;
        default:
            break;
    }
    return chunk;
}